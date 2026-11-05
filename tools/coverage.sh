#!/usr/bin/env bash
# coverage.sh [-o report-dir] [command...]
# Builds the coverage preset's flags in $TMPDIR, runs the commands (default: all test programs, with
# their default settings) from the build's bin directory, and reports which code they reached.
# A command is run as given, so `coverage.sh './dbat_test_bot 1 200'` measures just that.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
out="${TMPDIR:-/tmp}/dbat-coverage"
if [ "${1:-}" = "-o" ]; then out=$2; shift 2; fi
build="${TMPDIR:-/tmp}/dbat-coverage-build"

cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_C_FLAGS="--coverage -DDBAT_COVERAGE" -DCMAKE_EXE_LINKER_FLAGS="--coverage" > /dev/null
cmake --build "$build" -j"$(nproc)" -- --no-print-directory 2>&1 | grep -E ' error |\*\*\*' || true
find "$build" -name '*.gcda' -delete

work=$(mktemp -d "${TMPDIR:-/tmp}/dbat-coverage-run.XXXXXX")
trap 'rm -rf "${work:?}"' EXIT
export TMPDIR="$work"
if [ $# -eq 0 ]; then set -- ./dbat_test_game_world ./dbat_test_world ./dbat_test_commands ./dbat_test_bot; fi
for command in "$@"; do
  echo "coverage: running $command"
  (cd "$build/bin" && $command > /dev/null 2>&1) || echo "coverage: $command exited non-zero"
done

mkdir -p "$out"
python3 "$root/tools/coverage_report.py" "$root" "$build" "$out"
