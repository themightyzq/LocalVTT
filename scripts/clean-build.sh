#!/bin/bash

# Clean build script for LocalVTT - removes all build artifacts

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "================================================"
echo "LocalVTT - Clean Build"
echo "================================================"

cd "$PROJECT_DIR"

# Remove build directory
if [ -d "build" ]; then
    echo "Removing build directory..."
    rm -rf build
    echo "✅ Build directory removed"
else
    echo "Build directory doesn't exist (already clean)"
fi

# Remove any CMake cache files that might exist in project root
if [ -f "CMakeCache.txt" ]; then
    echo "Removing stray CMakeCache.txt..."
    rm -f CMakeCache.txt
fi

if [ -d "CMakeFiles" ]; then
    echo "Removing stray CMakeFiles..."
    rm -rf CMakeFiles
fi

echo ""
echo "✅ Clean complete!"
echo ""
echo "To rebuild, run:"
echo "  ./scripts/build.sh (Unix/Linux)"
echo "  ./scripts/build-and-run.command (macOS)"
echo "  scripts\\build.bat (Windows)"