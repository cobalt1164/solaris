#!/bin/bash
set -e

# Create build directory
mkdir -p build
cd build

# Configure build with CMake
echo "Configuring project with CMake..."
cmake ..

# Build project
echo "Building project..."
cmake --build .

echo "Build successful! Executable is located in bin/"
