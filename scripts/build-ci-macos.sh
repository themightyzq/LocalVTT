#!/bin/bash
set -e

# CI Build Script for macOS - No GUI launch
# Performs complete Qt bundling for distribution

echo "================================================"
echo "LocalVTT - CI Build Script (macOS)"
echo "================================================"

# Color output helpers
status() { echo "[✓] $1"; }
error() { echo "[✗] $1" >&2; }
warning() { echo "[⚠] $1"; }

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

cd "$PROJECT_DIR"

# Detect Qt installation
echo ""
echo "Detecting Qt installation..."
if [ -d "/opt/homebrew/opt/qt" ]; then
    QT_PREFIX="/opt/homebrew/opt/qt"
    status "Found Qt at $QT_PREFIX (Apple Silicon)"
elif [ -d "/usr/local/opt/qt" ]; then
    QT_PREFIX="/usr/local/opt/qt"
    status "Found Qt at $QT_PREFIX (Intel Mac)"
else
    error "Qt not found! Install with: brew install qt"
    exit 1
fi

QT_LIB_DIR="$QT_PREFIX/lib"
QT_PLUGINS_DIR="$QT_PREFIX/share/qt/plugins"
status "Qt plugins at: $QT_PLUGINS_DIR"

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo ""
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Configure with CMake
echo ""
echo "Configuring project with CMake..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT_PREFIX" \
    -DUSE_MACDEPLOYQT=OFF \
    -DSKIP_CODESIGN=ON

# Detect CPU count
CPU_COUNT=$(sysctl -n hw.ncpu)

# Build
echo ""
echo "Building LocalVTT with $CPU_COUNT parallel jobs..."
cmake --build . --config Release -j$CPU_COUNT

# Verify build succeeded
if [ ! -d "LocalVTT.app" ]; then
    error "Build failed - LocalVTT.app not found!"
    exit 1
fi
status "Build successful!"

# Now perform Qt bundling (extracted from build-and-run.command)
echo ""
echo "================================================"
echo "Deploying Qt Dependencies"
echo "================================================"

EXECUTABLE="$BUILD_DIR/LocalVTT.app/Contents/MacOS/LocalVTT"
FRAMEWORKS_DIR="$BUILD_DIR/LocalVTT.app/Contents/Frameworks"
PLUGINS_DIR="$BUILD_DIR/LocalVTT.app/Contents/PlugIns"
RESOURCES_DIR="$BUILD_DIR/LocalVTT.app/Contents/Resources"

# Qt frameworks to bundle
# NOTE: QtDBus must be included even though unused - QtGui links to it
QT_FRAMEWORKS=(
    "QtCore"
    "QtGui"
    "QtWidgets"
    "QtOpenGL"
    "QtOpenGLWidgets"
    "QtSvg"
    "QtDBus"
)

# Function to fix library paths
fix_qt_paths() {
    local binary="$1"
    local binary_name=$(basename "$binary")

    echo "  Fixing $binary_name..."

    # Fix Qt framework references
    for framework in "${QT_FRAMEWORKS[@]}"; do
        install_name_tool -change \
            "$QT_LIB_DIR/$framework.framework/Versions/A/$framework" \
            "@executable_path/../Frameworks/$framework.framework/Versions/A/$framework" \
            "$binary" 2>/dev/null || true

        install_name_tool -change \
            "@rpath/$framework.framework/Versions/A/$framework" \
            "@executable_path/../Frameworks/$framework.framework/Versions/A/$framework" \
            "$binary" 2>/dev/null || true

        install_name_tool -change \
            "@@HOMEBREW_PREFIX@@/opt/qt/lib/$framework.framework/Versions/A/$framework" \
            "@executable_path/../Frameworks/$framework.framework/Versions/A/$framework" \
            "$binary" 2>/dev/null || true
    done

    # Fix dependent library references
    local deps=$(otool -L "$binary" | grep -E "(libicu|libpcre|libz|libpng|libharfbuzz|libfreetype|libglib|libdouble|libb2|libmd4c|libgraphite|libintl|libgthread)" | awk '{print $1}')
    for dep in $deps; do
        local dep_name=$(basename "$dep")
        if [[ "$dep" == @executable_path* ]]; then
            continue
        fi
        install_name_tool -change \
            "$dep" \
            "@executable_path/../Frameworks/$dep_name" \
            "$binary" 2>/dev/null || true
    done

    # Special fix for JPEG plugin @@HOMEBREW_PREFIX@@ placeholder
    if otool -L "$binary" 2>/dev/null | grep -q "@@HOMEBREW_PREFIX@@.*libjpeg"; then
        install_name_tool -change \
            "@@HOMEBREW_PREFIX@@/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
            "/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
            "$binary" 2>/dev/null || true
    fi
}

# Clean any previous deployment
echo ""
echo "Cleaning previous deployment..."
rm -rf "$FRAMEWORKS_DIR" "$PLUGINS_DIR"
mkdir -p "$FRAMEWORKS_DIR"
mkdir -p "$PLUGINS_DIR/platforms"
mkdir -p "$PLUGINS_DIR/imageformats"
mkdir -p "$PLUGINS_DIR/iconengines"
mkdir -p "$PLUGINS_DIR/styles"
mkdir -p "$RESOURCES_DIR"

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

        FW_BINARY="$FRAMEWORKS_DIR/$framework.framework/Versions/A/$framework"
        if [ -f "$FW_BINARY" ]; then
            install_name_tool -id \
                "@executable_path/../Frameworks/$framework.framework/Versions/A/$framework" \
                "$FW_BINARY"
            fix_qt_paths "$FW_BINARY"
        fi
    fi
done

# Copy dependent libraries
echo ""
echo "Copying dependent libraries..."
REQUIRED_LIBS=(
    "libicui18n.77.1.dylib"
    "libicuuc.77.1.dylib"
    "libicudata.77.1.dylib"
    "libpcre2-16.0.dylib"
    "libpcre2-8.0.dylib"
    "libzstd.1.5.7.dylib"
    "libpng16.16.dylib"
    "libharfbuzz.0.dylib"
    "libfreetype.6.dylib"
    "libglib-2.0.0.dylib"
    "libdouble-conversion.3.3.0.dylib"
    "libb2.1.dylib"
    "libmd4c.0.5.2.dylib"
    "libgraphite2.3.2.1.dylib"
    "libintl.8.dylib"
    "libgthread-2.0.0.dylib"
    "libdbus-1.3.dylib"
)

for lib in "${REQUIRED_LIBS[@]}"; do
    HOMEBREW_LIB=$(find /opt/homebrew/Cellar /usr/local/Cellar -name "$lib" -type f 2>/dev/null | head -1)
    if [ -n "$HOMEBREW_LIB" ] && [ -f "$HOMEBREW_LIB" ]; then
        status "Copying $lib..."
        cp "$HOMEBREW_LIB" "$FRAMEWORKS_DIR/"
        fix_qt_paths "$FRAMEWORKS_DIR/$lib"
    fi
done

# Create library symlinks
echo ""
echo "Creating library symlinks..."
cd "$FRAMEWORKS_DIR"
if [ -f "libicui18n.77.1.dylib" ]; then ln -sf libicui18n.77.1.dylib libicui18n.77.dylib; fi
if [ -f "libicuuc.77.1.dylib" ]; then ln -sf libicuuc.77.1.dylib libicuuc.77.dylib; fi
if [ -f "libicudata.77.1.dylib" ]; then ln -sf libicudata.77.1.dylib libicudata.77.dylib; fi
if [ -f "libzstd.1.5.7.dylib" ]; then ln -sf libzstd.1.5.7.dylib libzstd.1.dylib; fi
if [ -f "libdouble-conversion.3.3.0.dylib" ]; then ln -sf libdouble-conversion.3.3.0.dylib libdouble-conversion.3.dylib; fi
if [ -f "libmd4c.0.5.2.dylib" ]; then ln -sf libmd4c.0.5.2.dylib libmd4c.0.dylib; fi
if [ -f "libgraphite2.3.2.1.dylib" ]; then ln -sf libgraphite2.3.2.1.dylib libgraphite2.3.dylib; fi
if [ -f "libharfbuzz.0.dylib" ]; then ln -sf libharfbuzz.0.dylib libharfbuzz.dylib; fi
if [ -f "libfreetype.6.dylib" ]; then ln -sf libfreetype.6.dylib libfreetype.dylib; fi

cd "$BUILD_DIR"

# Fix libz references to use system library
echo ""
echo "Configuring system libraries..."
for fw in QtWidgets QtSvg QtCore QtGui QtOpenGL QtOpenGLWidgets; do
    FW_BINARY="$FRAMEWORKS_DIR/$fw.framework/Versions/A/$fw"
    if [ -f "$FW_BINARY" ]; then
        install_name_tool -change \
            "@executable_path/../Frameworks/libz.1.dylib" \
            "/usr/lib/libz.1.dylib" \
            "$FW_BINARY" 2>/dev/null || true
    fi
done

for lib in "$FRAMEWORKS_DIR"/*.dylib; do
    if [ -f "$lib" ]; then
        if otool -L "$lib" | grep -q "libz.1.dylib"; then
            status "Fixing libz in $(basename $lib)..."
            install_name_tool -change \
                "@executable_path/../Frameworks/libz.1.dylib" \
                "/usr/lib/libz.1.dylib" \
                "$lib" 2>/dev/null || true
        fi
    fi
done

# Copy Qt plugins
echo ""
echo "Copying Qt plugins..."

# Platform plugin (critical)
if [ -f "$QT_PLUGINS_DIR/platforms/libqcocoa.dylib" ]; then
    status "Copying cocoa platform plugin..."
    cp "$QT_PLUGINS_DIR/platforms/libqcocoa.dylib" "$PLUGINS_DIR/platforms/"
    fix_qt_paths "$PLUGINS_DIR/platforms/libqcocoa.dylib"
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

# Icon engine plugin
if [ -f "$QT_PLUGINS_DIR/iconengines/libqsvgicon.dylib" ]; then
    cp "$QT_PLUGINS_DIR/iconengines/libqsvgicon.dylib" "$PLUGINS_DIR/iconengines/"
    fix_qt_paths "$PLUGINS_DIR/iconengines/libqsvgicon.dylib"
fi

# Style plugins
if [ -f "$QT_PLUGINS_DIR/styles/libqmacstyle.dylib" ]; then
    cp "$QT_PLUGINS_DIR/styles/libqmacstyle.dylib" "$PLUGINS_DIR/styles/" 2>/dev/null || true
    if [ -f "$PLUGINS_DIR/styles/libqmacstyle.dylib" ]; then
        fix_qt_paths "$PLUGINS_DIR/styles/libqmacstyle.dylib"
    fi
fi

# Fix main executable
echo ""
echo "Fixing main executable..."
fix_qt_paths "$EXECUTABLE"

# Create qt.conf
echo ""
echo "Creating qt.conf..."
cat > "$RESOURCES_DIR/qt.conf" << EOF
[Paths]
Plugins = PlugIns
Imports = Resources/qml
Qml2Imports = Resources/qml
EOF

# Code signing
echo ""
echo "Code signing application..."
codesign --force --deep --sign - "$BUILD_DIR/LocalVTT.app" 2>/dev/null || true

echo ""
echo "================================================"
echo "Build Complete!"
echo "================================================"
echo "App Bundle: $BUILD_DIR/LocalVTT.app"
echo "Frameworks: $(ls -1 $FRAMEWORKS_DIR/*.framework 2>/dev/null | wc -l | xargs) Qt frameworks bundled"
echo "Libraries: $(ls -1 $FRAMEWORKS_DIR/*.dylib 2>/dev/null | wc -l | xargs) dependent libraries bundled"
echo "Plugins: $(find $PLUGINS_DIR -name '*.dylib' 2>/dev/null | wc -l | xargs) Qt plugins bundled"
echo ""
echo "✅ CI build successful!"
