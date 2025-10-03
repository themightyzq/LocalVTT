#!/bin/bash

# macOS convenience wrapper for build.sh with library path fixing

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "================================================"
echo "LocalVTT - macOS Build and Run"
echo "================================================"

# Call the generic build script
"$SCRIPT_DIR/build.sh"

# macOS-specific: Fix Qt library paths if needed
cd "$PROJECT_DIR/build"

if [ -f "./LocalVTT.app/Contents/MacOS/LocalVTT" ]; then
    EXECUTABLE="./LocalVTT.app/Contents/MacOS/LocalVTT"

    # Find Qt installation
    QT_PATH=$(brew --prefix qt 2>/dev/null || echo "")

    if [ -n "$QT_PATH" ] && [ -d "$QT_PATH" ]; then
        echo "Fixing Qt library paths..."

        # Fix library paths with placeholders
        for framework in QtWidgets QtCore QtGui QtSvg QtOpenGL QtOpenGLWidgets; do
            install_name_tool -change "@@HOMEBREW_PREFIX@@/opt/qt/lib/${framework}.framework/Versions/A/${framework}" \
                                     "${QT_PATH}/lib/${framework}.framework/Versions/A/${framework}" \
                                     "$EXECUTABLE" 2>/dev/null || true
            install_name_tool -change "@rpath/${framework}.framework/Versions/A/${framework}" \
                                     "${QT_PATH}/lib/${framework}.framework/Versions/A/${framework}" \
                                     "$EXECUTABLE" 2>/dev/null || true
        done

        # Fix plugin paths
        for plugin in ./LocalVTT.app/Contents/PlugIns/**/*.dylib; do
            if [ -f "$plugin" ]; then
                for framework in QtCore QtGui; do
                    install_name_tool -change "@rpath/${framework}.framework/Versions/A/${framework}" \
                                             "${QT_PATH}/lib/${framework}.framework/Versions/A/${framework}" \
                                             "$plugin" 2>/dev/null || true
                done

                # Fix JPEG library dependency for DD2VTT support
                install_name_tool -change "@@HOMEBREW_PREFIX@@/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
                                         "/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
                                         "$plugin" 2>/dev/null || true
            fi
        done

        # Re-sign
        codesign --deep --force -s - ./LocalVTT.app 2>/dev/null || true
    fi

    # Kill existing instances
    pkill -f LocalVTT 2>/dev/null || true
fi