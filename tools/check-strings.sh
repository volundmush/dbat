#!/usr/bin/env bash
# check-strings.sh: text is std::string, std::string& or std::string_view, and the standard library does the work.
# This fails on any line of the game (libs/dbat/src, libs/dbat/include, apps) that uses a C string: a C string or
# memory call (strcpy, strdup, malloc, free, atoi, sscanf, sprintf, strcmp, strcasecmp, strstr, strlen, memcpy and
# the like), a char array, or a char pointer, outside the exceptions below. Comments and string literals don't count.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root" || exit 1

# <file> <lines>: deliberate exceptions, the lines a file may have and why. "all" lets off the whole file.
allowed='
libs/dbat/src/net/platform_posix.cpp  all  the system calls on POSIX, which take and give C buffers
libs/dbat/src/net/platform_win32.cpp  all  the system calls on Windows, likewise
libs/dbat/src/io/platform.cpp         all  the file system calls, likewise
apps/server/src/main.cpp              1    main(argc, argv): the system hands over the arguments as C strings
libs/dbat/src/io/reader.cpp           1    strtof() says where it stopped by a char pointer
libs/dbat/src/olc/cedit.cpp           1    likewise
libs/dbat/src/net/messages.cpp        1    act() takes text for $T, $t and $F in a slot that may hold a character or
                                           an object instead, as a C string behind a const void *
'

calls='(^|[^_.>[:alnum:]])(std::)?(strn?cpy|strn?cat|v?s?n?printf|strn?dup|malloc|calloc|realloc|free|atoi|atol|atoll|sscanf|strtok|strn?cmp|strn?casecmp|strstr|strr?chr|strlen|memcpy|memmove|memset|memcmp)[[:space:]]*\('
arrays='(^|[^_[:alnum:]])char[[:space:]]+(\*[[:space:]]*)?[_[:alnum:]]+[[:space:]]*\['
pointers='(^|[^_[:alnum:]])char[[:space:]]*\*'

fail=0
while IFS= read -r file; do
  limit=$(awk -v f="$file" '$1 == f {print $2}' <<< "$allowed")
  [ "$limit" = "all" ] && continue
  # the file without its comments and string literals, line numbers kept
  hits=$(perl -0777 -pe 's{/\*.*?\*/}{ my $c = $&; $c =~ s/[^\n]//g; $c }gse; s{//[^\n]*}{}g; s{"(?:[^"\\\n]|\\.)*"}{""}g; s{'"'"'(?:[^'"'"'\\\n]|\\.)+'"'"'}{'"''"'}g' "$file" |
         grep -nE "$calls|$arrays|$pointers")
  [ -n "$hits" ] || continue
  n=$(grep -c . <<< "$hits")
  [ -n "$limit" ] && [ "$n" -le "$limit" ] && continue
  while IFS= read -r hit; do
    echo "C string: $file:$hit"
  done <<< "$hits"
  [ -n "$limit" ] && echo "  ($file may have $limit)"
  fail=1
done < <(find libs/dbat/src libs/dbat/include apps -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | sort)

[ $fail -eq 0 ] && echo "check-strings: OK"
exit $fail
