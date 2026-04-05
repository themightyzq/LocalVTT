#!/bin/bash

# Build and Run Script for Crit VTT
# This script builds the project and launches the application

set -e

# Get the directory of this script (which IS the project directory)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$SCRIPT_DIR"

echo "================================================"
echo "Crit VTT - Build and Run"
echo "================================================"
echo "Project directory: $PROJECT_DIR"

# Change to project directory
cd "$PROJECT_DIR"

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

# Enter build directory
cd build

# Configure with CMake if not already configured
if [ ! -f "CMakeCache.txt" ]; then
    echo "Configuring project with CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
fi

# Build the project
echo "Building Crit VTT..."
BUILD_RESULT=0
cmake --build . -j8 || BUILD_RESULT=$?

# CRITICAL FIX: Remove AGL framework from link command if build failed
if [ $BUILD_RESULT -ne 0 ]; then
    if [ -f "CMakeFiles/CritVTT.dir/link.txt" ]; then
        if grep -q "\-framework AGL" CMakeFiles/CritVTT.dir/link.txt; then
            echo "Filtering out deprecated AGL framework..."
            # Remove "-framework AGL" from link.txt
            sed -i '' 's/-framework AGL//g' CMakeFiles/CritVTT.dir/link.txt

            echo "Re-linking without AGL framework..."
            # Force re-link with corrected link.txt
            rm -f ./CritVTT.app/Contents/MacOS/CritVTT
            cmake --build . --target CritVTT -j8 || BUILD_RESULT=$?
        fi
    fi
fi

# Check if build succeeded
if [ $BUILD_RESULT -eq 0 ] && [ -f "./CritVTT.app/Contents/MacOS/CritVTT" ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "Launching Crit VTT..."
    echo "================================================"

    # Fix Qt library paths with placeholders
    echo "Fixing Qt library paths..."
    EXECUTABLE="./CritVTT.app/Contents/MacOS/CritVTT"

    # Find Qt installation path
    QT_PATH="/opt/homebrew/Cellar/qt/6.9.2"
    if [ ! -d "$QT_PATH" ]; then
        # Try alternate version
        QT_PATH="/opt/homebrew/Cellar/qt/6.9.1"
    fi
    if [ ! -d "$QT_PATH" ]; then
        # Try to find Qt dynamically
        QT_PATH=$(brew --prefix qt 2>/dev/null || echo "/opt/homebrew/opt/qt")
    fi
    echo "Using Qt at: $QT_PATH"

    # Fix all library paths with @@HOMEBREW_PREFIX@@ placeholder
    install_name_tool -change "@@HOMEBREW_PREFIX@@/opt/qt/lib/QtWidgets.framework/Versions/A/QtWidgets" \
                             "${QT_PATH}/lib/QtWidgets.framework/Versions/A/QtWidgets" "$EXECUTABLE" 2>/dev/null || true

    install_name_tool -change "@@HOMEBREW_PREFIX@@/opt/qt/lib/QtSvg.framework/Versions/A/QtSvg" \
                             "${QT_PATH}/lib/QtSvg.framework/Versions/A/QtSvg" "$EXECUTABLE" 2>/dev/null || true

    install_name_tool -change "@rpath/QtWidgets.framework/Versions/A/QtWidgets" \
                             "${QT_PATH}/lib/QtWidgets.framework/Versions/A/QtWidgets" "$EXECUTABLE" 2>/dev/null || true

    install_name_tool -change "@rpath/QtCore.framework/Versions/A/QtCore" \
                             "${QT_PATH}/lib/QtCore.framework/Versions/A/QtCore" "$EXECUTABLE" 2>/dev/null || true

    install_name_tool -change "@rpath/QtGui.framework/Versions/A/QtGui" \
                             "${QT_PATH}/lib/QtGui.framework/Versions/A/QtGui" "$EXECUTABLE" 2>/dev/null || true

    # Remove code signature (it gets invalidated by install_name_tool anyway)
    echo "Removing code signature..."
    codesign --remove-signature "$EXECUTABLE" 2>/dev/null || true

    # Fix plugin library paths
    echo "Fixing Qt plugin paths..."
    for plugin in ./CritVTT.app/Contents/PlugIns/imageformats/*.dylib; do
        if [ -f "$plugin" ]; then
            # Fix Qt framework paths
            install_name_tool -change "@rpath/QtGui.framework/Versions/A/QtGui" \
                                     "${QT_PATH}/lib/QtGui.framework/Versions/A/QtGui" "$plugin" 2>/dev/null || true
            install_name_tool -change "@rpath/QtCore.framework/Versions/A/QtCore" \
                                     "${QT_PATH}/lib/QtCore.framework/Versions/A/QtCore" "$plugin" 2>/dev/null || true

            # Fix JPEG library dependency (critical for DD2VTT support)
            install_name_tool -change "@@HOMEBREW_PREFIX@@/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
                                     "/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.8.dylib" "$plugin" 2>/dev/null || true
        fi
    done

    # Also fix platform plugin paths if present
    for plugin in ./CritVTT.app/Contents/PlugIns/platforms/*.dylib; do
        if [ -f "$plugin" ]; then
            install_name_tool -change "@rpath/QtGui.framework/Versions/A/QtGui" \
                                     "${QT_PATH}/lib/QtGui.framework/Versions/A/QtGui" "$plugin" 2>/dev/null || true
            install_name_tool -change "@rpath/QtCore.framework/Versions/A/QtCore" \
                                     "${QT_PATH}/lib/QtCore.framework/Versions/A/QtCore" "$plugin" 2>/dev/null || true
        fi
    done

    # Ad-hoc sign the entire app bundle to prevent Killed: 9
    echo "Ad-hoc signing app bundle..."
    codesign --deep --force -s - ./CritVTT.app 2>/dev/null || true

    # Kill any existing instances
    pkill -f "CritVTT.app" 2>/dev/null || true
    sleep 0.5

    # Launch the application
    echo "Starting application..."
    open ./CritVTT.app
else
    echo ""
    echo "❌ Build failed!"
    exit 1
fi
