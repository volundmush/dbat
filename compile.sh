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
# enable the Python extension target, and buirmld incrementally.
BDIR="build"
#CC="gcc"
#CXX="g++"
#export CC CXX

PROFILE_MODE=${DBAT_PROFILE:-1}
cmake_args=(-S . -B "$BDIR" -G Ninja -DCMAKE_BUILD_TYPE=${MODE})

cmake "${cmake_args[@]}"

# Allow overriding the parallelism via the second argument; default to half the cores.
NPROC=$(nproc)
DEFAULT_JOBS=$(( NPROC / 2 ))
if [[ $DEFAULT_JOBS -lt 1 ]]; then
	DEFAULT_JOBS=1
fi
JOBS="${2:-$DEFAULT_JOBS}"

TARGET="${3:-all}"
build_cmd=(cmake --build "$BDIR" --target "$TARGET" --parallel "$JOBS")
if [[ "$PROFILE_MODE" == "1" ]]; then
	build_cmd+=(-- -d stats)
fi

"${build_cmd[@]}" || exit $?

exit 0
