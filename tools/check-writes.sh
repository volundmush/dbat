#!/usr/bin/env bash
# check-writes.sh: every file the game rewrites must go through io::text_writer (dbat/io/writer.hpp),
# so a crash or full disk can never leave it half-written. This lists any fopen() with a write,
# append or update mode outside the known exceptions below, and fails if it finds one.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root/libs/dbat/src" || exit 1

# <file> <function>: deliberate raw writes, each with the reason it is safe
allowed='
io/writer.cpp replace_file                      the writer itself (writes <path>.tmp, then renames it over <path>)
io/writer.cpp append_file                       append-only logs (news, bug/idea/typo, requests): a crash can only cut the last entry
io/paths.cpp touch                              appends nothing; only creates a missing file
net/comm.cpp open_logfile                       the syslog
'

fail=0
while IFS=: read -r file line _; do
  file=${file#./}
  func=$(awk -v L="$line" 'NR <= L && /^[A-Za-z_].*\(/ && !/;[ \t]*$/ { n = $0 } NR == L { print n; exit }' "$file" |
    sed -E 's/^ACMD\(([a-z_0-9]+)\).*/\1/; s/\(.*//; s/.*[ *]//')
  if ! grep -qE "^$file $func( |$)" <<< "$allowed"; then
    echo "raw write outside io::text_writer: $file:$line ($func)"
    fail=1
  fi
done < <(grep -rnE 'fopen\([^;]*,\s*"[wa]|fopen\([^;]*,\s*"r\+|\bfreopen\(|\bcreat\(|O_WRONLY|O_RDWR' --include='*.cpp' .)
[ $fail -eq 0 ] && echo "check-writes: OK"
exit $fail
