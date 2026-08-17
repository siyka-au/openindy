#!/bin/sh
# Launches the CMake-built openIndy GUI with a real display (not the
# QT_QPA_PLATFORM=offscreen mode the test suite uses). config/plugins/example
# are already populated alongside the binary by CMakeLists.txt's post-build
# copy step - nothing extra needed here.
set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/../build}"
BIN_DIR="$BUILD_DIR/bin"

if [ ! -x "$BIN_DIR/openIndy" ]; then
    echo "openIndy binary not found at $BIN_DIR/openIndy - build first, or set BUILD_DIR." >&2
    exit 1
fi

cd "$BIN_DIR"
LD_LIBRARY_PATH="$BIN_DIR:$LD_LIBRARY_PATH" exec ./openIndy "$@"
