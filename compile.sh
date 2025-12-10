#!/bin/bash

source .venv/bin/activate

case "${1,,}" in
debug)
echo "Compiling as debug...";
MODE=Debug
;;
release)
echo "Compiling as release..."
MODE=Release
;;
reldeb)
echo "Compiling as release with debug info..."
MODE=RelWithDebInfo
;;
*)
echo "Error: Must be called as debug, release, or reldeb!";
exit 1
;;
esac

# Default behavior: use the same CMake build dir as VS Code (./build),
# enable the Python extension target, and build incrementally.
BDIR="build"
cmake -S . -B "$BDIR" -G Ninja -DCMAKE_BUILD_TYPE=${MODE} -DDBAT_BUILD_PYTHON=ON

rm -rf $BDIR/libs/ext

# Allow overriding the parallelism via the second argument; default to half the cores.
NPROC=$(nproc)
DEFAULT_JOBS=$(( NPROC / 2 ))
if [[ $DEFAULT_JOBS -lt 1 ]]; then
	DEFAULT_JOBS=1
fi
JOBS="${2:-$DEFAULT_JOBS}"

# Build just the Python module by default for speed; pass "all" (third argument) to build everything.
TARGET="${3:-all}"
if [[ "${1,,}" == "release" || "${1,,}" == "reldeb" || "${1,,}" == "debug" ]]; then
	cmake --build "$BDIR" --target "$TARGET" --parallel "$JOBS" || exit $?
else
	cmake --build "$BDIR" --target "$TARGET" --parallel "$JOBS" || exit $?
fi

exit 0