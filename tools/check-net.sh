#!/usr/bin/env bash
# check-net.sh: the networking, the game loop and the server's start (src/net, util/log.cpp, apps/server) are modern
# C++: this fails on a C string or memory call (strcpy, strcat, sprintf, strdup, malloc, free, atoi, sscanf and the
# like) or a char array buffer, outside the exceptions below. Comments don't count. System calls are
# check-portable.sh's to catch.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root" || exit 1

# <file>: deliberate exceptions, each with the reason
allowed='
libs/dbat/src/net/platform_posix.cpp  the system calls on POSIX, which take and give C buffers
libs/dbat/src/net/platform_win32.cpp  the system calls on Windows, likewise
'

calls='(^|[^_.>[:alnum:]])(std::)?(strn?cpy|strn?cat|v?s?n?printf|strdup|strndup|malloc|calloc|realloc|free|atoi|atol|atoll|sscanf|strtok)[[:space:]]*\('
arrays='(^|[^_[:alnum:]])char[[:space:]]+(\*[[:space:]]*)?[_[:alnum:]]+[[:space:]]*\['

fail=0
while IFS= read -r file; do
  grep -qE "^${file}( |$)" <<< "$allowed" && continue
  # the file without its comments and string literals, line numbers kept
  stripped=$(perl -0777 -pe 's{/\*.*?\*/}{ my $c = $&; $c =~ s/[^\n]//g; $c }gse; s{//[^\n]*}{}g; s{"(?:[^"\\\n]|\\.)*"}{""}g' "$file")
  while IFS= read -r hit; do
    [ -n "$hit" ] || continue
    echo "C-ism: $file:$hit"
    fail=1
  done < <(grep -nE "$calls|$arrays" <<< "$stripped")
done < <( (find libs/dbat/src/net apps/server/src -name '*.cpp' -o -name '*.hpp'; echo libs/dbat/src/util/log.cpp) | sort)
[ $fail -eq 0 ] && echo "check-net: OK"
exit $fail
