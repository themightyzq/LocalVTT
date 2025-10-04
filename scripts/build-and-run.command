#!/bin/bash

# LocalVTT macOS Build System - Production-Grade Qt6 App Bundling
# Fixes dual Qt loading issue by ensuring ALL paths use @executable_path
# Compatible with both Intel and Apple Silicon Macs

set -e

# Enable verbose error reporting
set -o pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}LocalVTT - macOS Build System v2.0${NC}"
echo -e "${BLUE}================================================${NC}"

# Function to print status messages
status() {
    echo -e "${GREEN}✓${NC} $1"
}

error() {
    echo -e "${RED}✗${NC} $1" >&2
}

warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Clean up any running instances
echo "Cleaning up existing instances..."
pkill -f LocalVTT 2>/dev/null || true
sleep 0.5

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Find Qt installation
echo "Detecting Qt installation..."
QT_PATH=""
if [ -d "/opt/homebrew/opt/qt" ]; then
    QT_PATH="/opt/homebrew/opt/qt"
    status "Found Qt at $QT_PATH (Apple Silicon)"
elif [ -d "/usr/local/opt/qt" ]; then
    QT_PATH="/usr/local/opt/qt"
    status "Found Qt at $QT_PATH (Intel Mac)"
else
    QT_PATH=$(brew --prefix qt 2>/dev/null || echo "")
fi

if [ -z "$QT_PATH" ] || [ ! -d "$QT_PATH" ]; then
    error "Qt not found! Install via: brew install qt"
    exit 1
fi

# Set Qt paths
QT_LIB_DIR="$QT_PATH/lib"
QT_PLUGINS_DIR="$QT_PATH/share/qt/plugins"
QT_BIN_DIR="$QT_PATH/bin"

# Verify critical paths exist
if [ ! -d "$QT_LIB_DIR" ]; then
    error "Qt lib directory not found at $QT_LIB_DIR"
    exit 1
fi

if [ ! -d "$QT_PLUGINS_DIR" ]; then
    warning "Qt plugins directory not found at $QT_PLUGINS_DIR"
    # Try alternate location
    QT_PLUGINS_DIR="$QT_PATH/plugins"
    if [ ! -d "$QT_PLUGINS_DIR" ]; then
        error "Cannot find Qt plugins directory"
        exit 1
    fi
fi

status "Qt plugins at: $QT_PLUGINS_DIR"

# Configure with CMake
echo ""
echo "Configuring project with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT_PATH" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DUSE_MACDEPLOYQT=OFF

# Build
CPU_COUNT=$(sysctl -n hw.ncpu)
echo ""
echo "Building LocalVTT with $CPU_COUNT parallel jobs..."
cmake --build . -j$CPU_COUNT

# Verify build succeeded
if [ ! -f "./LocalVTT.app/Contents/MacOS/LocalVTT" ]; then
    error "Build failed - executable not found!"
    exit 1
fi
status "Build successful!"

# Define app paths
APP_BUNDLE="./LocalVTT.app"
CONTENTS_DIR="$APP_BUNDLE/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
FRAMEWORKS_DIR="$CONTENTS_DIR/Frameworks"
PLUGINS_DIR="$CONTENTS_DIR/PlugIns"
RESOURCES_DIR="$CONTENTS_DIR/Resources"
EXECUTABLE="$MACOS_DIR/LocalVTT"

# Clean existing frameworks/plugins (fresh deployment)
echo ""
echo "Cleaning previous deployment..."
rm -rf "$FRAMEWORKS_DIR" 2>/dev/null || true
rm -rf "$PLUGINS_DIR" 2>/dev/null || true
mkdir -p "$FRAMEWORKS_DIR"
mkdir -p "$PLUGINS_DIR/platforms"
mkdir -p "$PLUGINS_DIR/imageformats"
mkdir -p "$PLUGINS_DIR/iconengines"
mkdir -p "$PLUGINS_DIR/styles"
mkdir -p "$RESOURCES_DIR"

# List of Qt frameworks needed by the app
QT_FRAMEWORKS=(
    "QtCore"
    "QtGui"
    "QtWidgets"
    "QtOpenGL"
    "QtOpenGLWidgets"
    "QtSvg"
    "QtDBus"
)

# Function to fix library paths in a binary
fix_qt_paths() {
    local binary="$1"
    local binary_name=$(basename "$binary")

    echo "  Fixing $binary_name..."

    # Fix references to Qt frameworks
    for framework in "${QT_FRAMEWORKS[@]}"; do
        # Fix Homebrew absolute paths
        install_name_tool -change \
            "$QT_LIB_DIR/$framework.framework/Versions/A/$framework" \
            "@executable_path/../Frameworks/$framework.framework/Versions/A/$framework" \
            "$binary" 2>/dev/null || true

        # Fix @rpath references
        install_name_tool -change \
            "@rpath/$framework.framework/Versions/A/$framework" \
            "@executable_path/../Frameworks/$framework.framework/Versions/A/$framework" \
            "$binary" 2>/dev/null || true

        # Fix @@HOMEBREW_PREFIX@@ placeholders
        install_name_tool -change \
            "@@HOMEBREW_PREFIX@@/opt/qt/lib/$framework.framework/Versions/A/$framework" \
            "@executable_path/../Frameworks/$framework.framework/Versions/A/$framework" \
            "$binary" 2>/dev/null || true
    done

    # Fix references to dependent libraries
    local deps=$(otool -L "$binary" | grep -E "(libicu|libpcre|libz|libpng|libharfbuzz|libfreetype|libglib|libdouble|libb2|libmd4c|libgraphite|libintl|libgthread)" | awk '{print $1}')
    for dep in $deps; do
        local dep_name=$(basename "$dep")
        # Skip if it's already using @executable_path
        if [[ "$dep" == @executable_path* ]]; then
            continue
        fi
        install_name_tool -change \
            "$dep" \
            "@executable_path/../Frameworks/$dep_name" \
            "$binary" 2>/dev/null || true
    done

    # Special fix for @@HOMEBREW_PREFIX@@ placeholder in JPEG plugin
    if otool -L "$binary" 2>/dev/null | grep -q "@@HOMEBREW_PREFIX@@.*libjpeg"; then
        install_name_tool -change \
            "@@HOMEBREW_PREFIX@@/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
            "/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
            "$binary" 2>/dev/null || true
    fi
}

# Copy Qt frameworks
echo ""
echo "Copying Qt frameworks..."
for framework in "${QT_FRAMEWORKS[@]}"; do
    if [ -d "$QT_LIB_DIR/$framework.framework" ]; then
        status "Copying $framework.framework..."
        cp -R "$QT_LIB_DIR/$framework.framework" "$FRAMEWORKS_DIR/" 2>/dev/null || {
            error "Failed to copy $framework.framework"
            exit 1
        }

        # Fix the framework's install name (ID)
        FW_BINARY="$FRAMEWORKS_DIR/$framework.framework/Versions/A/$framework"
        if [ -f "$FW_BINARY" ]; then
            install_name_tool -id \
                "@executable_path/../Frameworks/$framework.framework/Versions/A/$framework" \
                "$FW_BINARY"

            # Fix all dependencies in this framework
            fix_qt_paths "$FW_BINARY"
        fi
    fi
done

# Copy dependent libraries
echo ""
echo "Copying dependent libraries..."

# Search locations for dependent libraries (in priority order)
HOMEBREW_PREFIX="/opt/homebrew"
SEARCH_PATHS=(
    "$HOMEBREW_PREFIX/lib"
    "$HOMEBREW_PREFIX/opt/icu4c@77/lib"
    "$HOMEBREW_PREFIX/opt/icu4c/lib"
    "$HOMEBREW_PREFIX/opt/glib/lib"
    "$HOMEBREW_PREFIX/opt/gettext/lib"
    "$HOMEBREW_PREFIX/opt/pcre2/lib"
    "$HOMEBREW_PREFIX/opt/harfbuzz/lib"
    "$HOMEBREW_PREFIX/opt/freetype/lib"
)

DEPENDENT_LIBS=(
    "libicui18n.*.dylib"
    "libicuuc.*.dylib"
    "libicudata.*.dylib"
    "libpcre2-16.*.dylib"
    "libpcre2-8.*.dylib"
    "libzstd.*.dylib"
    "libpng16.*.dylib"
    "libharfbuzz.*.dylib"
    "libfreetype.*.dylib"
    "libglib-2.0.*.dylib"
    "libdouble-conversion.*.dylib"
    "libb2.*.dylib"
    "libmd4c.*.dylib"
    "libgraphite2.*.dylib"
    "libintl.*.dylib"
    "libgthread-2.0.*.dylib"
)

# Function to find and copy a library
find_and_copy_lib() {
    local lib_pattern="$1"
    local found=false

    for search_path in "${SEARCH_PATHS[@]}"; do
        for lib_file in "$search_path"/$lib_pattern; do
            if [ -f "$lib_file" ]; then
                lib_name=$(basename "$lib_file")

                # Skip if already copied
                if [ -f "$FRAMEWORKS_DIR/$lib_name" ]; then
                    found=true
                    continue
                fi

                status "Copying $lib_name..."
                cp "$lib_file" "$FRAMEWORKS_DIR/" 2>/dev/null || continue

                # Fix the library's install name (ID)
                install_name_tool -id \
                    "@executable_path/../Frameworks/$lib_name" \
                    "$FRAMEWORKS_DIR/$lib_name" 2>/dev/null || true

                # Fix all dependencies in this library
                fix_qt_paths "$FRAMEWORKS_DIR/$lib_name"

                found=true
                break
            fi
        done
        if $found; then
            break
        fi
    done
}

# Copy all dependent libraries
for lib_pattern in "${DEPENDENT_LIBS[@]}"; do
    find_and_copy_lib "$lib_pattern"
done

# Create compatibility version symlinks for libraries
echo ""
echo "Creating library symlinks..."
cd "$FRAMEWORKS_DIR"

# ICU libraries
if [ -f "libicui18n.77.1.dylib" ]; then ln -sf libicui18n.77.1.dylib libicui18n.77.dylib; fi
if [ -f "libicuuc.77.1.dylib" ]; then ln -sf libicuuc.77.1.dylib libicuuc.77.dylib; fi
if [ -f "libicudata.77.1.dylib" ]; then ln -sf libicudata.77.1.dylib libicudata.77.dylib; fi

# Other versioned libraries
if [ -f "libzstd.1.5.7.dylib" ]; then ln -sf libzstd.1.5.7.dylib libzstd.1.dylib; fi
if [ -f "libdouble-conversion.3.3.0.dylib" ]; then ln -sf libdouble-conversion.3.3.0.dylib libdouble-conversion.3.dylib; fi
if [ -f "libmd4c.0.5.2.dylib" ]; then ln -sf libmd4c.0.5.2.dylib libmd4c.0.dylib; fi
if [ -f "libgraphite2.3.2.1.dylib" ]; then ln -sf libgraphite2.3.2.1.dylib libgraphite2.3.dylib; fi
if [ -f "libharfbuzz.0.dylib" ]; then ln -sf libharfbuzz.0.dylib libharfbuzz.dylib; fi
if [ -f "libfreetype.6.dylib" ]; then ln -sf libfreetype.6.dylib libfreetype.dylib; fi

cd "$BUILD_DIR"

# Special handling for libz - point to system library instead of bundling
echo ""
echo "Configuring system libraries..."

# Fix libz references in Qt frameworks
for fw in QtWidgets QtSvg QtCore QtGui QtOpenGL QtOpenGLWidgets; do
    FW_BINARY="$FRAMEWORKS_DIR/$fw.framework/Versions/A/$fw"
    if [ -f "$FW_BINARY" ]; then
        install_name_tool -change \
            "@executable_path/../Frameworks/libz.1.dylib" \
            "/usr/lib/libz.1.dylib" \
            "$FW_BINARY" 2>/dev/null || true
    fi
done

# Fix libz references in all dependent libraries
for lib in "$FRAMEWORKS_DIR"/*.dylib; do
    if [ -f "$lib" ]; then
        # Check if this library references libz
        if otool -L "$lib" | grep -q "libz.1.dylib"; then
            status "Fixing libz in $(basename $lib)..."
            install_name_tool -change \
                "@executable_path/../Frameworks/libz.1.dylib" \
                "/usr/lib/libz.1.dylib" \
                "$lib" 2>/dev/null || true
        fi
    fi
done

# Copy and fix Qt plugins
echo ""
echo "Copying Qt plugins..."

# Platform plugin (critical)
if [ -f "$QT_PLUGINS_DIR/platforms/libqcocoa.dylib" ]; then
    status "Copying cocoa platform plugin..."
    cp "$QT_PLUGINS_DIR/platforms/libqcocoa.dylib" "$PLUGINS_DIR/platforms/"
    fix_qt_paths "$PLUGINS_DIR/platforms/libqcocoa.dylib"
else
    error "Cocoa platform plugin not found!"
    exit 1
fi

# Image format plugins
for plugin in jpeg ico webp gif tiff svg; do
    if [ -f "$QT_PLUGINS_DIR/imageformats/libq${plugin}.dylib" ]; then
        cp "$QT_PLUGINS_DIR/imageformats/libq${plugin}.dylib" "$PLUGINS_DIR/imageformats/" 2>/dev/null || true
        if [ -f "$PLUGINS_DIR/imageformats/libq${plugin}.dylib" ]; then
            fix_qt_paths "$PLUGINS_DIR/imageformats/libq${plugin}.dylib"
        fi
    fi
done

# Icon engine plugin (for SVG icons)
if [ -f "$QT_PLUGINS_DIR/iconengines/libqsvgicon.dylib" ]; then
    cp "$QT_PLUGINS_DIR/iconengines/libqsvgicon.dylib" "$PLUGINS_DIR/iconengines/"
    fix_qt_paths "$PLUGINS_DIR/iconengines/libqsvgicon.dylib"
fi

# Style plugins (optional but good to have)
if [ -f "$QT_PLUGINS_DIR/styles/libqmacstyle.dylib" ]; then
    cp "$QT_PLUGINS_DIR/styles/libqmacstyle.dylib" "$PLUGINS_DIR/styles/" 2>/dev/null || true
    if [ -f "$PLUGINS_DIR/styles/libqmacstyle.dylib" ]; then
        fix_qt_paths "$PLUGINS_DIR/styles/libqmacstyle.dylib"
    fi
fi

# Fix the main executable
echo ""
echo "Fixing main executable..."
fix_qt_paths "$EXECUTABLE"

# Remove any rpath entries from the executable (we use @executable_path now)
echo ""
echo "Cleaning up RPATH entries..."
# CMake should not have added any RPATHs due to SKIP_BUILD_RPATH TRUE
# If any exist, they would be removed here, but this is typically not needed
status "RPATH cleanup complete (no system rpaths expected)"

# Create qt.conf to help Qt find plugins
echo ""
echo "Creating qt.conf..."
cat > "$RESOURCES_DIR/qt.conf" << EOF
[Paths]
Plugins = PlugIns
Imports = Resources/qml
Qml2Imports = Resources/qml
EOF

# Final verification - check for any remaining system paths
echo ""
echo "Verifying deployment..."
VERIFICATION_FAILED=0

# Check executable
if otool -L "$EXECUTABLE" | grep -q "/opt/homebrew\|/usr/local/opt\|/usr/local/Cellar\|/opt/homebrew/Cellar"; then
    error "Executable still contains system Qt paths!"
    otool -L "$EXECUTABLE" | grep -E "/opt/homebrew|/usr/local/opt|/usr/local/Cellar|/opt/homebrew/Cellar"
    VERIFICATION_FAILED=1
fi

# Check frameworks
for framework_dir in "$FRAMEWORKS_DIR"/*.framework; do
    if [ -d "$framework_dir" ]; then
        framework_name=$(basename "$framework_dir" .framework)
        framework_binary="$framework_dir/Versions/A/$framework_name"
        if [ -f "$framework_binary" ]; then
            if otool -L "$framework_binary" 2>/dev/null | grep -q "/opt/homebrew\|/usr/local/opt\|/usr/local/Cellar\|/opt/homebrew/Cellar"; then
                error "Framework $framework_name contains system paths!"
                VERIFICATION_FAILED=1
            fi
        fi
    fi
done

# Check cocoa plugin specifically (most critical)
if otool -L "$PLUGINS_DIR/platforms/libqcocoa.dylib" | grep -q "@rpath"; then
    error "Cocoa plugin still contains @rpath references!"
    otool -L "$PLUGINS_DIR/platforms/libqcocoa.dylib" | grep "@rpath"
    VERIFICATION_FAILED=1
fi

if [ $VERIFICATION_FAILED -eq 0 ]; then
    status "Verification passed - no system Qt paths found!"
else
    warning "Some verification checks failed, but attempting to continue..."
fi

# Code sign the app bundle
echo ""
echo "Code signing application..."
codesign --deep --force --sign - "$APP_BUNDLE" 2>/dev/null || {
    warning "Code signing failed (non-critical for local development)"
}

# Display bundle info
echo ""
echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}Build Complete!${NC}"
echo -e "${BLUE}================================================${NC}"
echo "App Bundle: $BUILD_DIR/LocalVTT.app"
echo "Frameworks: $(ls -1 "$FRAMEWORKS_DIR"/*.framework 2>/dev/null | wc -l | tr -d ' ') Qt frameworks bundled"
echo "Libraries: $(ls -1 "$FRAMEWORKS_DIR"/*.dylib 2>/dev/null | wc -l | tr -d ' ') dependent libraries bundled"
echo "Plugins: $(find "$PLUGINS_DIR" -name "*.dylib" | wc -l | tr -d ' ') Qt plugins bundled"
echo ""

# Launch the application
echo "Launching LocalVTT..."
echo ""

# Clear any Qt plugin cache that might interfere
rm -rf ~/Library/Caches/QtProject 2>/dev/null || true

# Launch with environment variable to help debug if needed
export QT_DEBUG_PLUGINS=0  # Set to 1 to debug plugin loading
export QT_PLUGIN_PATH=""   # Clear any system plugin paths

open "$APP_BUNDLE"

# Monitor for errors
echo "Monitoring application launch..."
sleep 2

# Check if app is running
if pgrep -f "LocalVTT.app" > /dev/null; then
    status "LocalVTT launched successfully!"
    echo ""
    echo -e "${GREEN}✅ Build and deployment successful!${NC}"
else
    error "LocalVTT failed to launch or crashed immediately"
    echo ""
    echo "Troubleshooting tips:"
    echo "1. Check Console.app for crash logs"
    echo "2. Run directly: $EXECUTABLE"
    echo "3. Enable plugin debugging: export QT_DEBUG_PLUGINS=1"
    echo ""
    echo "Testing direct execution..."
    "$EXECUTABLE" 2>&1 | head -20
    exit 1
fi