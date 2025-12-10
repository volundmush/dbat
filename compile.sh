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

PROFILE_MODE=${DBAT_PROFILE:-1}
cmake_args=(-S . -B "$BDIR" -G Ninja -DCMAKE_BUILD_TYPE=${MODE} -DDBAT_BUILD_PYTHON=ON)
if [[ "$PROFILE_MODE" == "1" ]]; then
	cmake_args+=(-DDBAT_BUILD_PROFILE=ON)
else
	cmake_args+=(-DDBAT_BUILD_PROFILE=OFF)
fi

cmake "${cmake_args[@]}"

rm -rf $BDIR/libs/ext

# Allow overriding the parallelism via the second argument; default to half the cores.
NPROC=$(nproc)
DEFAULT_JOBS=$(( NPROC / 2 ))
if [[ $DEFAULT_JOBS -lt 1 ]]; then
	DEFAULT_JOBS=1
fi
JOBS="${2:-$DEFAULT_JOBS}"

# Build just the Python module by default for speed; pass "all" (third argument) to build everything.
TARGET="${3:-dbat_ext}"
build_cmd=(cmake --build "$BDIR" --target "$TARGET" --parallel "$JOBS")
if [[ "$PROFILE_MODE" == "1" ]]; then
	build_cmd+=(-- -d stats)
fi

"${build_cmd[@]}" || exit $?

# Seamless import: symlink the built extension into the active venv's site-packages
SO_SRC=$(ls -t compiled/dbat_ext*.so 2>/dev/null | head -n 1)
if [[ -n "$SO_SRC" ]]; then
	SITEPKG=$(python -c 'import site,sys; print(site.getsitepackages()[0])')
	SO_BASENAME=$(basename "$SO_SRC")
	SO_DEST="$SITEPKG/$SO_BASENAME"
	# Create/refresh symlink
	ln -sf "$(realpath "$SO_SRC")" "$SO_DEST"
	echo "Linked $SO_DEST -> $SO_SRC"
	# Optional convenience alias without tag for import tools that search by simple name
	ln -sf "$SO_BASENAME" "$SITEPKG/dbat_ext.so" 2>/dev/null || true
fi

if [[ "$PROFILE_MODE" == "1" ]]; then
	if command -v ninja >/dev/null 2>&1; then
		PROFILE_LOG="$BDIR/ninja-profile.log"
		ninja -C "$BDIR" -t profile | tee "$PROFILE_LOG"
		echo "Ninja profile written to $PROFILE_LOG"
	else
		echo "Warning: Ninja not found; skipping -t profile output" >&2
	fi
fi

exit 0