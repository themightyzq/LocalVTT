#!/bin/bash
set -e

# Build script for LocalVTT (macOS/Linux)
echo "================================================"
echo "LocalVTT - Build Script"
echo "================================================"

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Detect CPU count for parallel build
if command -v nproc &> /dev/null; then
    CPU_COUNT=$(nproc)
elif command -v sysctl &> /dev/null; then
    CPU_COUNT=$(sysctl -n hw.ncpu)
else
    CPU_COUNT=4
fi

# Build
echo "Building LocalVTT with $CPU_COUNT parallel jobs..."
cmake --build . -j$CPU_COUNT

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""

    # Launch based on platform
    if [ "$(uname)" = "Darwin" ]; then
        echo "Launching LocalVTT (macOS)..."
        ./LocalVTT.app/Contents/MacOS/LocalVTT
    else
        echo "Launching LocalVTT (Linux)..."
        ./LocalVTT
    fi
else
    echo "❌ Build failed!"
    exit 1
fi