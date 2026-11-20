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
# tools too, so that both builds take their own roundtrip.cpp and statedump.cpp
git -C "$root" archive "$ref" libs apps tools | tar -x -C "$work/ref-src"
# No inlining and no interprocedural optimisation, so each function's code depends only on its own
# source: otherwise e.g. making a function static changes how GCC specialises it and its callers.
flags=(-O2 -fno-inline -fno-ipa-sra -fno-ipa-cp -fno-ipa-ra -fno-ipa-icf -fno-ipa-pure-const
       -fno-ipa-reference -fno-ipa-modref -fno-ipa-vrp -fno-ipa-bit-cp -fno-ipa-stack-alignment
       -ffunction-sections -fdata-sections -Wno-builtin-macro-redefined -D__LINE__=0)
# One build at a time (each runs JOBS compilers, see build-tree.sh), to keep within memory
"$here/build-tree.sh" "$work/ref-src" "$work/ref" "${flags[@]}"
"$here/build-tree.sh" "$root" "$work/new" "${flags[@]}"
set +e
python3 "$here/objdiff.py" "$work/ref/obj" "$work/new/obj"; status=$?
# OBJDIFF_KEEP=1 keeps the two builds, for a closer look
[ -n "${OBJDIFF_KEEP:-}" ] && echo "objdiff: kept $work" || rm -rf "$work"
exit $status
