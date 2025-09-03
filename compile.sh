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

# Build just the Python module by default for speed; pass "all" to build everything.
TARGET="${2:-dbat_ext}"
if [[ "${1,,}" == "release" || "${1,,}" == "reldeb" || "${1,,}" == "debug" ]]; then
	cmake --build "$BDIR" --target "$TARGET" -j || exit $?
else
	cmake --build "$BDIR" --target "$TARGET" -j || exit $?
fi

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

exit 0