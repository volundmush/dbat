#!/usr/bin/env bash
# build-tree.sh <tree-dir> <out-dir> [extra cflags...]
# Compiles the server in <tree-dir> (a checkout containing libs/ and apps/) plus tools/roundtrip.cpp into
# <out-dir>/dbat, with objects in <out-dir>/obj. Used by verify.sh, objdiff.sh and deadcode.sh.
set -euo pipefail
tree=$(cd "$1" && pwd) out=$2; shift 2
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$out/obj"
# Trees from before the move to C++ are C.
if [ -f "$tree/apps/server/src/main.cpp" ]; then cc=g++ std=gnu++26 ext=cpp; else cc=gcc std=gnu23 ext=c; fi
# __FILE__ is made relative to the tree, so builds of different checkouts are comparable byte for byte
cflags=(-std=$std -g -w -DPATH_MAX=4096 -fmacro-prefix-map="$tree/=" -fmacro-prefix-map="$here/=tools/"
        -I"$tree/libs/dbat/include" "$@")
# The helper calls server functions, so it must match the tree being built.
roundtrip="$tree/tools/roundtrip.$ext"
[ -f "$roundtrip" ] || roundtrip="$here/roundtrip.cpp"
# At most JOBS compilers at once (default 4): one per file at once ran the machine out of memory
jobs=${JOBS:-4} running=0 fail=0
while IFS= read -r f; do
  obj="$out/obj/$(echo "${f#"$tree"/}" | tr / _).o"
  $cc "${cflags[@]}" -c "$f" -o "$obj" &
  running=$((running + 1))
  if [ $running -ge "$jobs" ]; then wait -n || fail=1; running=$((running - 1)); fi
done < <(find "$tree/libs/dbat/src" -name "*.$ext"; echo "$tree/apps/server/src/main.$ext"; echo "$roundtrip")
while [ $running -gt 0 ]; do wait -n || fail=1; running=$((running - 1)); done
[ $fail -eq 0 ] || { echo "build-tree: compile failed in $tree" >&2; exit 1; }
$cc -o "$out/dbat" "$out"/obj/*.o -lm -lcrypt ${LDFLAGS_EXTRA:-}
