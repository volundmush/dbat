#!/usr/bin/env bash
# verify.sh [--data DIR] [git-ref]   (default: data/ and HEAD)
# Behavioural regression check of the working tree against <git-ref>:
#   1. `dbat -c` syntax-check boot: exit status and log must match.
#   2. Full boot under gdb with the RNG pinned (mob stats are rolled from the seed), then
#      roundtrip_dump(): every player is saved, and every zone's mobs, objects, rooms, shops, guilds, triggers
#      and zone file, and the rest the game saves (see roundtrip.cpp); the data/ trees must match. Before that,
#      state_dump() writes what the boot loaded to statedump.txt (see statedump.cpp), which must match too.
#      VERIFY_SHARED_ROUNDTRIP=1 builds the ref with this roundtrip.cpp. A ref without a statedump.cpp is built with
#      this one.
# Each build runs on its own copy of data/; the real data/ is never written.
set -euo pipefail
datadir=
if [ "${1:-}" = --data ]; then datadir=$(cd "$2" && pwd); shift 2; fi
ref=${1:-HEAD}
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
here="$root/tools"
work=$(mktemp -d "${TMPDIR:-/tmp}/dbat-verify.XXXXXX")
echo "verify: $ref vs working tree in $work"
mkdir -p "$work/ref-src"
git -C "$root" archive "$ref" libs apps tools | tar -x -C "$work/ref-src"
# VERIFY_SHARED_ROUNDTRIP=1: build the ref with this tree's roundtrip.cpp, so that both save the same files. Only
# while the save functions it calls are the same on both sides.
[ "${VERIFY_SHARED_ROUNDTRIP:-0}" = 1 ] && cp "$here/roundtrip.cpp" "$work/ref-src/tools/roundtrip.cpp"
[ -f "$work/ref-src/tools/statedump.cpp" ] || cp "$here/statedump.cpp" "$work/ref-src/tools/statedump.cpp"
datadir=${datadir:-$root/data}
# One build at a time (each runs JOBS compilers, see build-tree.sh), to keep within memory
"$here/build-tree.sh" "$work/ref-src" "$work/ref" -O2
"$here/build-tree.sh" "$root" "$work/new" -O2

port_ref=4198 port_new=4197
cat > "$work/rt.gdb.in" <<'GDB'
set pagination off
set confirm off
handle SIGPIPE nostop noprint pass
break boot_db
run -d data @PORT@ > boot.log 2>&1
call (void)@SRANDOM@
finish
call (long)state_dump(g)
call (long)@ROUNDTRIP@
kill
quit
GDB
for side in ref new; do
  port_var=port_$side
  # The ref may predate GameWorld; `finish` above leaves gdb in init_game, where g is in scope.
  tree=$root; [ $side = ref ] && tree=$work/ref-src
  call='roundtrip_dump()'; grep -qs 'roundtrip_dump(GameWorld' "$tree"/tools/roundtrip.c* && call='roundtrip_dump(g)'
  srandom='circle_srandom(12345)'; grep -qs 'circle_srandom(GameWorld' "$tree"/libs/dbat/include/dbat/random.h* && srandom='circle_srandom(g, 12345)'
  sed "s/@PORT@/${!port_var}/; s/@ROUNDTRIP@/$call/; s/@SRANDOM@/$srandom/" "$work/rt.gdb.in" > "$work/$side/rt.gdb"
done
for side in ref new; do
  (
    set +e
    mkdir -p "$work/$side/check" "$work/$side/run"
    mkdir -p "$work/$side/check/data" "$work/$side/run/data"
    cp -a "$datadir/." "$work/$side/check/data/" && cp -a "$datadir/." "$work/$side/run/data/"
    cd "$work/$side/check" && timeout 600 "$work/$side/dbat" -c -d data 9999 > check.log 2>&1; echo $? > check.rc
    cd "$work/$side/run" && timeout 1200 gdb -q -batch -x "$work/$side/rt.gdb" "$work/$side/dbat" > gdb.out 2>&1
  ) &
done
wait

norm() { sed -E 's/^[A-Z][a-z]{2} +[0-9]+ [0-9:]+ :: //; s/0x[0-9a-f]+/0xADDR/g; s/on port [0-9]+/on port N/; /^PCLEAN:/d' "$1"; }
status=0
for side in ref new; do echo "  $side: -c exit $(cat "$work/$side/check/check.rc"), $(grep -h ROUNDTRIP "$work/$side/run/boot.log" || echo 'ROUNDTRIP missing')"; done
[ "$(cat "$work/ref/check/check.rc")" = "$(cat "$work/new/check/check.rc")" ] || { echo "FAIL: -c exit status differs"; status=1; }
diff <(norm "$work/ref/check/check.log") <(norm "$work/new/check/check.log") > "$work/check.diff" || { echo "FAIL: -c logs differ ($work/check.diff)"; status=1; }
diff <(norm "$work/ref/run/boot.log") <(norm "$work/new/run/boot.log") > "$work/boot.diff" || { echo "FAIL: boot logs differ ($work/boot.diff)"; status=1; }
# *.bak: atomic saves of player data keep the replaced version beside the file (dbat/atomicfile.hpp)
diff "$work/ref/run/data/statedump.txt" "$work/new/run/data/statedump.txt" > "$work/state.diff" || { echo "FAIL: loaded state differs ($work/state.diff)"; status=1; }
diff -r -x '*.bak' -x statedump.txt "$work/ref/run/data" "$work/new/run/data" > "$work/data.diff" || { echo "FAIL: saved data differs ($work/data.diff)"; status=1; }
[ $status -eq 0 ] && echo "verify: OK" || echo "verify: differences kept in $work"
# VERIFY_KEEP=1 keeps the work directory even when nothing differs
[ $status -eq 0 ] && [ "${VERIFY_KEEP:-0}" != 1 ] && rm -rf "$work"
exit $status
