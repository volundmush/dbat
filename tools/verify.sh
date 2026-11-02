#!/usr/bin/env bash
# verify.sh [git-ref]   (default: HEAD)
# Behavioural regression check of the working tree against <git-ref>:
#   1. `dbat -c` syntax-check boot: exit status and log must match.
#   2. Full boot under gdb with the RNG pinned (mob stats are rolled from the seed), then
#      roundtrip_dump(): every player and every zone's mobs are saved; the data/ trees must match.
# Each build runs on its own copy of data/; the real data/ is never written.
set -euo pipefail
ref=${1:-HEAD}
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
here="$root/tools"
work=$(mktemp -d "${TMPDIR:-/tmp}/dbat-verify.XXXXXX")
echo "verify: $ref vs working tree in $work"
mkdir -p "$work/ref-src"
git -C "$root" archive "$ref" libs apps | tar -x -C "$work/ref-src"
"$here/build-tree.sh" "$work/ref-src" "$work/ref" -O2 & p1=$!
"$here/build-tree.sh" "$root" "$work/new" -O2 & p2=$!
wait $p1; wait $p2

port_ref=4198 port_new=4197
cat > "$work/rt.gdb.in" <<'GDB'
set pagination off
set confirm off
handle SIGPIPE nostop noprint pass
break boot_db
run -d data @PORT@ > boot.log 2>&1
call (void)circle_srandom(12345)
finish
call (long)roundtrip_dump()
kill
quit
GDB
for side in ref new; do
  port_var=port_$side
  sed "s/@PORT@/${!port_var}/" "$work/rt.gdb.in" > "$work/$side/rt.gdb"
done
for side in ref new; do
  (
    set +e
    mkdir -p "$work/$side/check" "$work/$side/run"
    cp -a "$root/data" "$work/$side/check/" && cp -a "$root/data" "$work/$side/run/"
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
# *.bak: atomic saves of player data keep the replaced version beside the file (dbat/atomicfile.h)
diff -r -x '*.bak' "$work/ref/run/data" "$work/new/run/data" > "$work/data.diff" || { echo "FAIL: saved data differs ($work/data.diff)"; status=1; }
[ $status -eq 0 ] && { echo "verify: OK"; rm -rf "$work"; } || echo "verify: differences kept in $work"
exit $status
