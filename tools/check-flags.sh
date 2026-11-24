#!/usr/bin/env bash
# check-flags.sh [--update]: flags are typed (an enum class per family, held in a flag_set<E>; see dbat/flag_set.hpp).
# This counts, per file of the game (libs/dbat/src, libs/dbat/include, apps) and its tests, the lines that still use
# the old flags: a bitvector_t array, the bit macros (IS_SET_AR, SET_BIT_AR, REMOVE_BIT_AR, TOGGLE_BIT_AR, IS_SET,
# SET_BIT, REMOVE_BIT, TOGGLE_BIT), or the old readers (sprintbit, sprinttype, sprintbitarray, get_flag_by_name,
# check_flags_by_name_ar, flag_array_set). Comments and string literals don't count.
#
# The families convert one at a time, so the counts are a ratchet: check-flags.baseline holds each file's count, and
# this fails if a file has more than its baseline (or any, if it isn't listed). After a change that removes some, run
# it with --update to lower the baseline to match.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root" || exit 1
baseline=tools/check-flags.baseline

macros='(^|[^_[:alnum:]])(IS_SET_AR|SET_BIT_AR|REMOVE_BIT_AR|TOGGLE_BIT_AR|IS_SET|SET_BIT|REMOVE_BIT|TOGGLE_BIT)[[:space:]]*\('
arrays='(^|[^_[:alnum:]])bitvector_t[[:space:]]+[_[:alnum:]]+[[:space:]]*\['
readers='(^|[^_[:alnum:]])(sprintbit|sprinttype|sprintbitarray|get_flag_by_name|check_flags_by_name_ar|flag_array_set)[[:space:]]*\('

# "<count> <file>" for every file with any
count_all() {
  while IFS= read -r file; do
    # the file without its comments and string literals
    n=$(perl -0777 -pe 's{/\*.*?\*/}{ my $c = $&; $c =~ s/[^\n]//g; $c }gse; s{//[^\n]*}{}g; s{"(?:[^"\\\n]|\\.)*"}{""}g; s{'"'"'(?:[^'"'"'\\\n]|\\.)+'"'"'}{'"''"'}g' "$file" |
        grep -cE "$macros|$arrays|$readers")
    [ "$n" -gt 0 ] && echo "$n $file"
  done < <(find libs/dbat/src libs/dbat/include libs/dbat/tests apps -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | sort)
}

current=$(count_all)

if [ "${1:-}" = "--update" ]; then
  { echo "# check-flags.sh's counts: <lines using the old flags> <file>. Lower it with tools/check-flags.sh --update."
    echo "$current"; } > "$baseline"
  echo "check-flags: baseline updated, $(awk '{n += $1} END {print n + 0}' <<< "$current") lines in $(grep -c . <<< "$current") files"
  exit 0
fi

fail=0
lower=0
while read -r n file; do
  [ -n "$file" ] || continue
  was=$(awk -v f="$file" '$2 == f {print $1}' "$baseline")
  if [ -z "$was" ]; then
    echo "old flags: $file has $n lines and no baseline (it may have none)"
    fail=1
  elif [ "$n" -gt "$was" ]; then
    echo "old flags: $file has $n lines, more than its baseline of $was"
    fail=1
  elif [ "$n" -lt "$was" ]; then
    lower=1
  fi
done <<< "$current"
# a file in the baseline that has none left, or is gone, can be lowered too
while read -r was file; do
  [[ "$was" =~ ^[0-9]+$ ]] || continue
  grep -qE "^[0-9]+ ${file}$" <<< "$current" || lower=1
done < "$baseline"

[ $lower -eq 1 ] && echo "check-flags: some files are below their baseline; lower it with tools/check-flags.sh --update"
[ $fail -eq 0 ] && echo "check-flags: OK, $(awk '{n += $1} END {print n + 0}' <<< "$current") lines left"
exit $fail
