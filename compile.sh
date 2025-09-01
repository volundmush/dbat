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

pip install -v --config-settings=cmake.build-type=$MODE .