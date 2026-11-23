#!/usr/bin/env bash
# check-olc.sh: OLC stays on its sessions and std::string. Fails if any of the old Oasis OLC machinery comes back:
# the shared oasis_olc_data or oasis.hpp, the OLC_*() access macros, cleanup_olc(), str_udup(), or an OLC function
# taking a C string to parse (char *arg).
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root/libs/dbat" || exit 1

fail=0
report() {
  echo "$1"
  fail=1
}

# the old machinery, anywhere in the game (string literals left out, but for #include's: a log line may still name a
# macro)
while IFS=: read -r file line text; do
  report "old OLC machinery: $file:$line: $text"
done < <(grep -rnE --include='*.cpp' --include='*.hpp' '' src include tests |
  sed -E '/^[^:]*:[0-9]+:#include/!s/"([^"\\]|\\.)*"/""/g' |
  grep -E '\bOLC_[A-Z_]+\(|\bOLC\(d\)|\boasis_olc_data\b|dbat/oasis\.hpp|\bcleanup_olc\b|\bstr_udup\b|\bCLEANUP_(ALL|STRUCTS|CONFIG)\b')

# C-string parsers in OLC
while IFS=: read -r file line text; do
  func=$(sed -E 's/\(.*//; s/.*[ *&]//' <<< "$text")
  report "OLC parses a C string: $file:$line ($func)"
done < <(grep -rnE '\(.*\bchar \*(arg|argument|str|string)\b' src/olc src/dg/dg_olc.cpp include/dbat/olc include/dbat/obj_edit.hpp)

[ $fail -eq 0 ] && echo "check-olc: OK"
exit $fail
