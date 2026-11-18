#!/usr/bin/env bash
# check-portable.sh: the file layer (src/io) and the loaders and savers (src/db, with the world files in src/db/world)
# use only standard C++ for files: std::filesystem, the io:: reader and writer, <cstdio>. This fails on any POSIX or
# Linux header, or a call to a POSIX or glibc function that has a standard or io:: replacement. Comments don't count.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root/libs/dbat/src" || exit 1

# <file>: deliberate exceptions, each with the reason
allowed='
io/platform.cpp     pushing a file out to the disk (fsync/_commit), which standard C++ has no call for
'

headers='#include <(unistd\.h|dirent\.h|fcntl\.h|sys/[a-z_]+\.h|linux/[a-z_]+\.h)>'
calls='(^|[^:_.>[:alnum:]])(getline|strlcpy|strlcat|strcasecmp|strncasecmp|fsync|stat|lstat|access|unlink|chdir|mkdir|opendir|readdir)[[:space:]]*\('

fail=0
while IFS= read -r file; do
  grep -qE "^${file}( |$)" <<< "$allowed" && continue
  # the file without its comments, line numbers kept
  stripped=$(perl -0777 -pe 's{/\*.*?\*/}{ my $c = $&; $c =~ s/[^\n]//g; $c }gse; s{//[^\n]*}{}g' "$file")
  while IFS= read -r hit; do
    [ -n "$hit" ] || continue
    echo "not portable: $file:$hit"
    fail=1
  done < <(grep -nE "$headers|$calls" <<< "$stripped")
done < <(find io db -name '*.cpp' -o -name '*.hpp' | sort)
[ $fail -eq 0 ] && echo "check-portable: OK"
exit $fail
