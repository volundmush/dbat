#!/usr/bin/env sh
set -eu

# Build helper intended to run inside the Ubuntu 24.04 dev container.
# It configures and builds dbat with CMake + Ninja using clang/clang++.

ROOT_DIR="${ROOT_DIR:-/workspace/dbat}"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
GENERATOR="${GENERATOR:-Ninja}"
JOBS="${JOBS:-}"

if [ ! -d "$ROOT_DIR" ]; then
  echo "error: ROOT_DIR does not exist: $ROOT_DIR" >&2
  exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "error: cmake is not installed in this environment" >&2
  exit 1
fi

if ! command -v clang >/dev/null 2>&1; then
  echo "error: clang is not installed in this environment" >&2
  exit 1
fi

if ! command -v clang++ >/dev/null 2>&1; then
  echo "error: clang++ is not installed in this environment" >&2
  exit 1
fi

# Auto-detect parallelism if JOBS is not set
if [ -z "$JOBS" ]; then
  if command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
  else
    JOBS="4"
  fi
fi

echo "==> Configuring"
echo "    ROOT_DIR   : $ROOT_DIR"
echo "    BUILD_DIR  : $BUILD_DIR"
echo "    BUILD_TYPE : $BUILD_TYPE"
echo "    GENERATOR  : $GENERATOR"
echo "    JOBS       : $JOBS"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
  -G "$GENERATOR" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++

echo "==> Building"
cmake --build "$BUILD_DIR" --parallel "$JOBS"

echo "==> Done"
echo "Build directory: $BUILD_DIR"
