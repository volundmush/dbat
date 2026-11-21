#!/usr/bin/env bash
# check-windows.sh: the networking files that should build on Windows do, compiled for x86_64-windows-gnu with
# `zig c++` (Zig 0.17 or later; nothing is linked). There is no Windows build of the game: this keeps the platform
# layer (src/net/platform_win32.cpp) and the files on it honest. Skipped, with a note, if zig isn't installed.
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root/libs/dbat" || exit 1

# The files checked: all of the networking's but the POSIX layer. (copyover.cpp builds, and says it isn't available.)
files='
src/net/platform_win32.cpp
src/net/connection.cpp
src/net/input.cpp
src/net/output.cpp
src/net/colour.cpp
src/net/prompt.cpp
src/net/messages.cpp
src/net/game_loop.cpp
src/net/signals.cpp
src/net/copyover.cpp
'

if ! command -v zig > /dev/null; then
  echo "check-windows: skipped (no zig)"
  exit 0
fi
work=$(mktemp -d "${TMPDIR:-/tmp}/dbat-windows.XXXXXX")
trap 'rm -rf "$work"' EXIT
# zig keeps its cache here, not in the home directory. (-fsyntax-only would be quicker, but zig 0.17 then fails
# renaming the object it never wrote.)
export ZIG_LOCAL_CACHE_DIR="$work/cache" ZIG_GLOBAL_CACHE_DIR="$work/cache"
fail=0
for f in $files; do
  if ! zig c++ -target x86_64-windows-gnu -std=gnu++26 -Wall -Wextra -Wno-unused-parameter -Werror \
      -Iinclude -Isrc/net -c "$f" -o "$work/out.obj" > "$work/log" 2>&1; then
    echo "doesn't build for Windows: $f"
    sed 's/^/  /' "$work/log" | head -20
    fail=1
  fi
done
[ $fail -eq 0 ] && echo "check-windows: OK"
exit $fail
