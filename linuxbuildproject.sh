#!/bin/bash

BUILD_DIR="build"
GENERATOR="Unix Makefiles"
BUILD_TYPE="Debug"

if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring the project with CMake..."
cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..

echo "Building the project..."
cmake --build . --config "$BUILD_TYPE"

cd ..
echo "Build completed!"
