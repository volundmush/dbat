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

rm -f *.so
rm -f dbat_ext/*.cpp

OUTFOLDER=build

mkdir -p $OUTFOLDER
cmake -S . -B $OUTFOLDER -G Ninja -D CMAKE_BUILD_TYPE=$MODE
cmake --build $OUTFOLDER -- -j $(nproc)

python setup.py build_ext --inplace