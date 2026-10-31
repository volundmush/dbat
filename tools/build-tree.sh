#!/usr/bin/env bash
# build-tree.sh <tree-dir> <out-dir> [extra cflags...]
# Compiles the server in <tree-dir> (a checkout containing libs/ and apps/) plus tools/roundtrip.c into
# <out-dir>/dbat, with objects in <out-dir>/obj. Used by verify.sh, objdiff.sh and deadcode.sh.
set -euo pipefail
tree=$(cd "$1" && pwd) out=$2; shift 2
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$out/obj"
# __FILE__ is made relative to the tree, so builds of different checkouts are comparable byte for byte
cflags=(-std=gnu23 -g -w -DPATH_MAX=4096 -fmacro-prefix-map="$tree/=" -fmacro-prefix-map="$here/=tools/"
        -I"$tree/libs/dbat/include" "$@")
pids=()
while IFS= read -r f; do
  obj="$out/obj/$(echo "${f#"$tree"/}" | tr / _).o"
  gcc "${cflags[@]}" -c "$f" -o "$obj" & pids+=($!)
done < <(find "$tree/libs/dbat/src" -name '*.c'; echo "$tree/apps/server/src/main.c"; echo "$here/roundtrip.c")
fail=0; for p in "${pids[@]}"; do wait "$p" || fail=1; done
[ $fail -eq 0 ] || { echo "build-tree: compile failed in $tree" >&2; exit 1; }
gcc -o "$out/dbat" "$out"/obj/*.o -lm -lcrypt ${LDFLAGS_EXTRA:-}
