#!/usr/bin/env bash
# check-flags.sh: flags are typed (an enum class per family, held in a flag_set<E>; see dbat/flag_set.hpp). This fails
# if the game (libs/dbat/src, libs/dbat/include, apps) or its tests use the old flags again: a bitvector_t array, the
# bit macros (IS_SET_AR, SET_BIT_AR, REMOVE_BIT_AR, TOGGLE_BIT_AR, IS_SET, SET_BIT, REMOVE_BIT, TOGGLE_BIT), the old
# readers (sprintbit, sprinttype, sprintbitarray, get_flag_by_name, check_flags_by_name_ar, flag_array_set) or an
# *_ARRAY_MAX word count (a flag_set has FLAG_WORDS). Comments and string literals don't count.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root" || exit 1

macros='(^|[^_[:alnum:]])(IS_SET_AR|SET_BIT_AR|REMOVE_BIT_AR|TOGGLE_BIT_AR|IS_SET|SET_BIT|REMOVE_BIT|TOGGLE_BIT)[[:space:]]*\('
arrays='(^|[^_[:alnum:]])bitvector_t[[:space:]]+[_[:alnum:]]+[[:space:]]*\['
readers='(^|[^_[:alnum:]])(sprintbit|sprinttype|sprintbitarray|get_flag_by_name|check_flags_by_name_ar|flag_array_set)[[:space:]]*\('
counts='(^|[^_[:alnum:]])[A-Z]+_ARRAY_MAX([^_[:alnum:]]|$)'

fail=0
while IFS= read -r file; do
  # the file without its comments and string literals, each found line with its number
  found=$(perl -0777 -pe 's{/\*.*?\*/}{ my $c = $&; $c =~ s/[^\n]//g; $c }gse; s{//[^\n]*}{}g; s{"(?:[^"\\\n]|\\.)*"}{""}g; s{'"'"'(?:[^'"'"'\\\n]|\\.)+'"'"'}{'"''"'}g' "$file" |
      grep -nE "$macros|$arrays|$readers|$counts")
  if [ -n "$found" ]; then
    sed "s|^|old flags: $file:|" <<< "$found"
    fail=1
  fi
done < <(find libs/dbat/src libs/dbat/include libs/dbat/tests apps -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | sort)

[ $fail -eq 0 ] && echo "check-flags: OK"
exit $fail
