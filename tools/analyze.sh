#!/usr/bin/env bash
# analyze.sh: run GCC's static analyzer (-fanalyzer) over the server and list what it finds, grouped by kind.
# It follows each function's paths on its own, so it can't see an object being handed to a global list or a
# file closed by a helper: most leak reports are false, and other kinds can be too. Treat it as a list of
# places to look, not as failures. Takes about 20 seconds.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
work=$(mktemp -d "${TMPDIR:-/tmp}/dbat-analyze.XXXXXX")
find "$root/libs/dbat/src" "$root/apps/server/src" -name '*.cpp' |
  xargs -P "$(nproc)" -I{} sh -c '${CXX:-g++} -std=gnu++26 -O1 -fanalyzer -I"$1/libs/dbat/include" -c "$2" -o /dev/null 2>&1' \
    _ "$root" {} > "$work/out.txt"
# Only the analyzer's own warnings are kept: the build already reports the rest. (-w would hide these too.)
grep -E 'warning: .*\[-Wanalyzer-' "$work/out.txt" | sed "s|^$root/||" | sort -u > "$work/found.txt"
for kind in $(grep -oE '\[-Wanalyzer-[a-z-]+\]' "$work/found.txt" | sort | uniq -c | sort -n | awk '{print $2}' | tr -d '[]'); do
  echo "== ${kind#-W} ($(grep -c -- "\[$kind\]" "$work/found.txt"))"
  grep -- "\[$kind\]" "$work/found.txt" | sed -E 's/ \[CWE-[0-9]+\]//; s/ \[-Wanalyzer-[a-z-]+\]$//'
done
rm -rf "$work"
