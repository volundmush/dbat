#!/usr/bin/env bash
# deadcode.sh: list functions in the working tree that nothing reachable from main() calls.
# Links with --gc-sections and reports the discarded .text sections. Code that is compiled out by a
# constant condition (e.g. `if (false && ...)`) is invisible to this check, so a function it lists can
# still have callers in disabled code: the compiler will tell you if you delete one of those.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
work=$(mktemp -d "${TMPDIR:-/tmp}/dbat-deadcode.XXXXXX")
LDFLAGS_EXTRA="-Wl,--gc-sections,--print-gc-sections" \
  "$root/tools/build-tree.sh" "$root" "$work" -O0 -ffunction-sections -fdata-sections 2> "$work/gc.txt" || { cat "$work/gc.txt" >&2; exit 1; }
grep -o "removing unused section '\.text\.[^']*' in file '[^']*'" "$work/gc.txt" \
  | sed -E "s/.*'\.text\.([^']*)' in file '.*\/obj\/([^']*)\.o'/\2: \1/; s/^libs_dbat_src_//" \
  | grep -v 'roundtrip' | sort
rm -rf "$work"
