#!/usr/bin/env bash
# check-strings.sh: text is std::string, std::string& or std::string_view, and the standard library does the work.
# This counts, per file of the game (libs/dbat/src, libs/dbat/include, apps), the lines that still use a C string:
# a C string or memory call (strcpy, strdup, malloc, free, atoi, sscanf, strcmp, strcasecmp, strstr, strlen and
# the like), a char array, or a char pointer. Comments and string literals don't count.
#
# The C strings are being removed a phase at a time, so the counts are a ratchet: check-strings.baseline holds each
# file's count, and this fails if a file has more than its baseline (or any, if it isn't listed). After a change
# that removes some, run it with --update to lower the baseline to match.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root" || exit 1
baseline=tools/check-strings.baseline

# <file>: deliberate exceptions, each with the reason
allowed='
libs/dbat/src/net/platform_posix.cpp  the system calls on POSIX, which take and give C buffers
libs/dbat/src/net/platform_win32.cpp  the system calls on Windows, likewise
libs/dbat/src/io/platform.cpp         the file system calls, likewise
'

calls='(^|[^_.>[:alnum:]])(std::)?(strn?cpy|strn?cat|v?s?n?printf|strn?dup|malloc|calloc|realloc|free|atoi|atol|atoll|sscanf|strtok|strn?cmp|strn?casecmp|strstr|strr?chr|strlen|memcpy|memmove|memset|memcmp)[[:space:]]*\('
arrays='(^|[^_[:alnum:]])char[[:space:]]+(\*[[:space:]]*)?[_[:alnum:]]+[[:space:]]*\['
pointers='(^|[^_[:alnum:]])char[[:space:]]*\*'

# "<count> <file>" for every file with any
count_all() {
  while IFS= read -r file; do
    grep -qE "^${file}( |$)" <<< "$allowed" && continue
    # the file without its comments and string literals
    n=$(perl -0777 -pe 's{/\*.*?\*/}{ my $c = $&; $c =~ s/[^\n]//g; $c }gse; s{//[^\n]*}{}g; s{"(?:[^"\\\n]|\\.)*"}{""}g; s{'"'"'(?:[^'"'"'\\\n]|\\.)+'"'"'}{'"''"'}g' "$file" |
        grep -cE "$calls|$arrays|$pointers")
    [ "$n" -gt 0 ] && echo "$n $file"
  done < <(find libs/dbat/src libs/dbat/include apps -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | sort)
}

current=$(count_all)

if [ "${1:-}" = "--update" ]; then
  { echo "# check-strings.sh's counts: <lines using a C string> <file>. Lower it with tools/check-strings.sh --update."
    echo "$current"; } > "$baseline"
  echo "check-strings: baseline updated, $(awk '{n += $1} END {print n + 0}' <<< "$current") lines in $(grep -c . <<< "$current") files"
  exit 0
fi

fail=0
lower=0
while read -r n file; do
  [ -n "$file" ] || continue
  was=$(awk -v f="$file" '$2 == f {print $1}' "$baseline")
  if [ -z "$was" ]; then
    echo "C strings: $file has $n lines and no baseline (it may have none)"
    fail=1
  elif [ "$n" -gt "$was" ]; then
    echo "C strings: $file has $n lines, more than its baseline of $was"
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

[ $lower -eq 1 ] && echo "check-strings: some files are below their baseline; lower it with tools/check-strings.sh --update"
[ $fail -eq 0 ] && echo "check-strings: OK, $(awk '{n += $1} END {print n + 0}' <<< "$current") lines left"
exit $fail
