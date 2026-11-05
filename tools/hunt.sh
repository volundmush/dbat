#!/usr/bin/env bash
# hunt.sh [hunt.py options]: builds the ASan preset and hunts for bugs on every core.
# Findings go to build-asan/hunt/ unless --out says otherwise. See tools/README.md.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
cd "$root"
cmake --preset asan > /dev/null
cmake --build build-asan -j"$(nproc)" --target dbat_test_bot
exec python3 tools/hunt.py --binary build-asan/bin/dbat_test_bot --out build-asan/hunt "$@"
