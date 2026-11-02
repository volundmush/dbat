#!/usr/bin/env bash
# check-writes.sh: every file the game rewrites must go through atomic_open() (dbat/atomicfile.h),
# so a crash or full disk can never leave it half-written. This lists any fopen() with a write,
# append or update mode outside the known exceptions below, and fails if it finds one.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root/libs/dbat/src" || exit 1

# <file> <function>: deliberate raw writes, each with the reason it is safe
allowed='
util/atomicfile.c atomic_open                 the helper itself (writes <path>.tmp)
db/fileop.c touch                             appends nothing; only creates a missing file
olc/tedit.c news_string_cleanup               append-only news log: a crash can only cut the last entry
commands/other/player.c do_gen_write          append-only bug/idea/typo logs
commands/other/rp.c log_imm_action            append-only log
commands/other/rp.c log_custom                append-only log
commands/other/rp.c do_rpp                    append-only log
net/comm.c open_logfile                       the syslog
commands/act.wizard.c execute_copyover        transient: written just before exec and read straight after
db/db.c auc_save                              auction code, left alone on purpose
'

fail=0
while IFS=: read -r file line _; do
  file=${file#./}
  func=$(awk -v L="$line" 'NR <= L && /^[A-Za-z_].*\(/ && !/;[ \t]*$/ { n = $0 } NR == L { print n; exit }' "$file" |
    sed -E 's/^ACMD\(([a-z_0-9]+)\).*/\1/; s/\(.*//; s/.*[ *]//')
  if ! grep -qE "^$file $func( |$)" <<< "$allowed"; then
    echo "raw write outside atomic_open(): $file:$line ($func)"
    fail=1
  fi
done < <(grep -rnE 'fopen\([^;]*,\s*"[wa]|fopen\([^;]*,\s*"r\+|\bfreopen\(|\bcreat\(|O_WRONLY|O_RDWR' --include='*.c' .)
[ $fail -eq 0 ] && echo "check-writes: OK"
exit $fail
