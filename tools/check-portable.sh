#!/usr/bin/env bash
# check-portable.sh: the file layer (src/io) and the loaders and savers (src/db, with the world files in src/db/world)
# use only standard C++ for files: std::filesystem, the io:: reader and writer, <cstdio>. This fails on any POSIX or
# Linux header, or a call to a POSIX or glibc function that has a standard or io:: replacement. Comments don't count.
# The networking (src/net) goes through its platform layer (dbat/net/platform.hpp) the same way: no system header
# and no socket call outside it.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root/libs/dbat/src" || exit 1

# <file>: deliberate exceptions, each with the reason
allowed='
io/platform.cpp       pushing a file out to the disk (fsync/_commit), which standard C++ has no call for
net/platform_posix.cpp  the networking layer on POSIX
net/platform_win32.cpp  the networking layer on Windows
net/signals.cpp       signals, the deadlock timer and the crash backtrace, until they move to the layer
net/copyover.cpp      the copyover exec and the sockets it brings across, which only Linux can do
'

headers='#include <(unistd\.h|dirent\.h|fcntl\.h|poll\.h|netdb\.h|execinfo\.h|winsock2?\.h|ws2tcpip\.h|windows\.h|sys/[a-z_]+\.h|linux/[a-z_]+\.h|netinet/[a-z_]+\.h|arpa/[a-z_]+\.h)>'
# file calls (src/io and src/db), and socket calls (src/net)
file_calls='(^|[^:_.>[:alnum:]])(getline|strlcpy|strlcat|strcasecmp|strncasecmp|fsync|stat|lstat|access|unlink|chdir|mkdir|opendir|readdir)[[:space:]]*\('
net_calls='(^|[^:_.>[:alnum:]])(socket|socketpair|setsockopt|getsockopt|accept|bind|listen|connect|poll|select|fcntl|ioctl|send|recv|read|write|close|inet_[a-z]+|getrlimit|setrlimit)[[:space:]]*\('

fail=0
check() { # check <calls> <dirs...>
  local calls=$1; shift
  while IFS= read -r file; do
    grep -qE "^${file}( |$)" <<< "$allowed" && continue
    # the file without its comments, line numbers kept
    stripped=$(perl -0777 -pe 's{/\*.*?\*/}{ my $c = $&; $c =~ s/[^\n]//g; $c }gse; s{//[^\n]*}{}g' "$file")
    while IFS= read -r hit; do
      [ -n "$hit" ] || continue
      echo "not portable: $file:$hit"
      fail=1
    done < <(grep -nE "$headers|$calls" <<< "$stripped")
  done < <(find "$@" -name '*.cpp' -o -name '*.hpp' | sort)
}
check "$file_calls" io db
check "$net_calls" net
[ $fail -eq 0 ] && echo "check-portable: OK"
exit $fail
