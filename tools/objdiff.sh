#!/usr/bin/env bash
# objdiff.sh [git-ref]   (default: HEAD)
# Code-equivalence check of the working tree against <git-ref>: compiles both with -O2 -fno-inline and
# __LINE__ pinned, then compares each function's normalised disassembly (see objdiff.py).
# Use it to show that a refactor (moves, renames, reformatting) changed no generated code.
set -euo pipefail
ref=${1:-HEAD}
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
here="$root/tools"
work=$(mktemp -d "${TMPDIR:-/tmp}/dbat-objdiff.XXXXXX")
mkdir -p "$work/ref-src"
git -C "$root" archive "$ref" libs apps | tar -x -C "$work/ref-src"
# No inlining and no interprocedural optimisation, so each function's code depends only on its own
# source: otherwise e.g. making a function static changes how GCC specialises it and its callers.
flags=(-O2 -fno-inline -fno-ipa-sra -fno-ipa-cp -fno-ipa-ra -fno-ipa-icf -fno-ipa-pure-const
       -fno-ipa-reference -fno-ipa-modref -fno-ipa-vrp -fno-ipa-bit-cp -fno-ipa-stack-alignment
       -ffunction-sections -fdata-sections -Wno-builtin-macro-redefined -D__LINE__=0)
"$here/build-tree.sh" "$work/ref-src" "$work/ref" "${flags[@]}" & p1=$!
"$here/build-tree.sh" "$root" "$work/new" "${flags[@]}" & p2=$!
wait $p1; wait $p2
set +e
python3 "$here/objdiff.py" "$work/ref/obj" "$work/new/obj"; status=$?
rm -rf "$work"
exit $status
