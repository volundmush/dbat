#!/usr/bin/env bash
# check-headers.sh: every header under libs/dbat/include must compile on its own (include what it uses).
set -uo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
inc="$root/libs/dbat/include"
fail=0
while IFS= read -r h; do
  rel=${h#"$inc"/}
  if ! echo "#include \"$rel\"" | ${CC:-clang} -fsyntax-only -std=gnu23 -I"$inc" -x c - 2> /dev/null; then
    echo "not self-contained: $rel"; fail=1
  fi
done < <(find "$inc" -name '*.h' | sort)
[ $fail -eq 0 ] && echo "check-headers: OK"
exit $fail
