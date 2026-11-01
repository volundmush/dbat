#!/usr/bin/env bash
# symbolize.sh: turn a logged backtrace (log_backtrace() / the fatal signal handler) into function names and file:line.
#
#   tools/symbolize.sh [binary] < syslog-excerpt
#
# backtrace_symbols_fd() can only name exported functions; static ones come out as bare offsets like
# "dbat(+0x1a2b3)[0x55d0...]". This resolves every frame from the binary's debug info instead. It only gives the
# right answer for the exact binary that wrote the log (same build, not a later rebuild).
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
bin=${1:-$root/build/bin/dbat}
[ -x "$bin" ] || { echo "symbolize: no executable at $bin" >&2; exit 1; }
name=$(basename "$bin")

# Symbol name -> address, for frames printed as "func+0x12".
declare -A sym
while read -r addr _ s; do sym[$s]=$addr; done < <(nm --defined-only "$bin" 2> /dev/null)

while IFS= read -r line; do
  # backtrace_symbols_fd format: <path>(<func>+0x<off>)[0x<abs>] or <path>(+0x<off>)[0x<abs>]
  if [[ $line =~ ([^ \(]*)\(([A-Za-z_][A-Za-z0-9_.]*)?\+0x([0-9a-fA-F]+)\)\ ?\[0x[0-9a-fA-F]+\] ]]; then
    path=${BASH_REMATCH[1]} func=${BASH_REMATCH[2]} off=$((16#${BASH_REMATCH[3]}))
    if [ "$(basename "$path")" != "$name" ]; then
      echo "    ${path##*/} ${func:-?}+0x${BASH_REMATCH[3]}"; continue
    fi
    if [ -n "$func" ]; then
      [ -n "${sym[$func]:-}" ] || { echo "    $func+0x${BASH_REMATCH[3]} (not in $name)"; continue; }
      addr=$((16#${sym[$func]} + off))
    else
      addr=$off
    fi
    # A frame's address is the return address; step back one byte so the line is the call itself.
    addr2line -f -i -C -p -e "$bin" "$(printf '0x%x' $((addr - 1)))" | sed "s|$root/||; s/^/    /"
  else
    echo "$line"
  fi
done
