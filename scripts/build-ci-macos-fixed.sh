#!/bin/bash
set -e

# CI Build Script for macOS - No GUI launch
# Performs complete Qt bundling for distribution
# FIXED: Proper library search paths for both Intel and Apple Silicon

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

# Detect Qt installation and architecture
echo ""
echo "Detecting Qt installation..."
if [ -d "/opt/homebrew/opt/qt" ]; then
    QT_PREFIX="/opt/homebrew/opt/qt"
    HOMEBREW_PREFIX="/opt/homebrew"
    ARCH="arm64"
    status "Found Qt at $QT_PREFIX (Apple Silicon)"
elif [ -d "/usr/local/opt/qt" ]; then
    QT_PREFIX="/usr/local/opt/qt"
    HOMEBREW_PREFIX="/usr/local"
    ARCH="x86_64"
    status "Found Qt at $QT_PREFIX (Intel Mac)"
else
    error "Qt not found! Install with: brew install qt"
    exit 1
fi

QT_LIB_DIR="$QT_PREFIX/lib"
QT_PLUGINS_DIR="$QT_PREFIX/share/qt/plugins"
status "Qt plugins at: $QT_PLUGINS_DIR"
status "Homebrew prefix: $HOMEBREW_PREFIX"
status "Architecture: $ARCH"

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

# Now perform Qt bundling
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

    # Fix dependent library references - check all possible locations
    local deps=$(otool -L "$binary" | grep -E "(libicu|libpcre|libz|libpng|libharfbuzz|libfreetype|libglib|libdouble|libb2|libmd4c|libgraphite|libintl|libgthread|libdbus)" | awk '{print $1}')
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
            "$HOMEBREW_PREFIX/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
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

# Copy dependent libraries - FIXED search paths
echo ""
echo "Copying dependent libraries..."

# Search locations for dependent libraries (in priority order)
SEARCH_PATHS=(
    "$HOMEBREW_PREFIX/lib"
    "$HOMEBREW_PREFIX/opt/icu4c@77/lib"
    "$HOMEBREW_PREFIX/opt/icu4c/lib"
    "$HOMEBREW_PREFIX/opt/glib/lib"
    "$HOMEBREW_PREFIX/opt/gettext/lib"
    "$HOMEBREW_PREFIX/opt/pcre2/lib"
    "$HOMEBREW_PREFIX/opt/harfbuzz/lib"
    "$HOMEBREW_PREFIX/opt/freetype/lib"
    "$HOMEBREW_PREFIX/opt/libpng/lib"
    "$HOMEBREW_PREFIX/opt/zstd/lib"
    "$HOMEBREW_PREFIX/opt/double-conversion/lib"
    "$HOMEBREW_PREFIX/opt/libb2/lib"
    "$HOMEBREW_PREFIX/opt/md4c/lib"
    "$HOMEBREW_PREFIX/opt/graphite2/lib"
    "$HOMEBREW_PREFIX/opt/dbus/lib"
)

# Also search in Cellar for versioned libraries
for cellar_path in "$HOMEBREW_PREFIX/Cellar"/*/*/lib; do
    if [ -d "$cellar_path" ]; then
        SEARCH_PATHS+=("$cellar_path")
    fi
done

REQUIRED_LIBS=(
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
    "libdbus-1.*.dylib"
)

# Function to find and copy a library
find_and_copy_lib() {
    local lib_pattern="$1"
    local found=false

    for search_path in "${SEARCH_PATHS[@]}"; do
        for lib_file in $search_path/$lib_pattern; do
            if [ -f "$lib_file" ]; then
                lib_name=$(basename "$lib_file")

                # Skip if already copied
                if [ -f "$FRAMEWORKS_DIR/$lib_name" ]; then
                    found=true
                    continue
                fi

                status "Copying $lib_name from $search_path..."
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

    if ! $found; then
        warning "Library pattern $lib_pattern not found in any search path"
    fi
}

# Copy all dependent libraries
for lib_pattern in "${REQUIRED_LIBS[@]}"; do
    find_and_copy_lib "$lib_pattern"
done

# Create library symlinks
echo ""
echo "Creating library symlinks..."
cd "$FRAMEWORKS_DIR"

# Create symlinks for all versioned libraries
for lib in *.dylib; do
    if [[ "$lib" =~ (.+)\.([0-9]+\.[0-9]+\.[0-9]+)\.dylib$ ]]; then
        base="${BASH_REMATCH[1]}"
        version="${BASH_REMATCH[2]}"
        major_version="${version%%.*}"

        # Create major version symlink
        ln -sf "$lib" "${base}.${major_version}.dylib" 2>/dev/null || true
        # Create base symlink
        ln -sf "$lib" "${base}.dylib" 2>/dev/null || true
    elif [[ "$lib" =~ (.+)\.([0-9]+\.[0-9]+)\.dylib$ ]]; then
        base="${BASH_REMATCH[1]}"
        version="${BASH_REMATCH[2]}"
        major_version="${version%%.*}"

        # Create major version symlink
        ln -sf "$lib" "${base}.${major_version}.dylib" 2>/dev/null || true
        # Create base symlink
        ln -sf "$lib" "${base}.dylib" 2>/dev/null || true
    fi
done

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

# Verification - check for missing dependencies
echo ""
echo "================================================"
echo "Verifying Bundle Integrity"
echo "================================================"

# Function to check for missing libraries
check_dependencies() {
    local binary="$1"
    local missing=0

    echo "Checking: $(basename $binary)"

    # Get all dependencies
    local deps=$(otool -L "$binary" 2>/dev/null | tail -n +2 | awk '{print $1}')

    for dep in $deps; do
        # Skip system libraries
        if [[ "$dep" == /System/* ]] || [[ "$dep" == /usr/lib/* ]]; then
            continue
        fi

        # Skip self-references
        if [[ "$dep" == *"$(basename $binary)"* ]]; then
            continue
        fi

        # Check if using @executable_path
        if [[ "$dep" == @executable_path/* ]]; then
            # Verify the file actually exists
            local resolved_path="${dep/@executable_path/$BUILD_DIR/LocalVTT.app/Contents/MacOS}"
            if [ ! -f "$resolved_path" ]; then
                error "  Missing: $dep"
                missing=$((missing + 1))
            fi
        elif [[ "$dep" == @rpath/* ]] || [[ "$dep" == /opt/* ]] || [[ "$dep" == /usr/local/* ]]; then
            error "  System dependency: $dep"
            missing=$((missing + 1))
        fi
    done

    return $missing
}

# Check main executable
TOTAL_MISSING=0
check_dependencies "$EXECUTABLE" || TOTAL_MISSING=$((TOTAL_MISSING + $?))

# Check all frameworks
for fw in "$FRAMEWORKS_DIR"/*.framework/Versions/A/*; do
    if [ -f "$fw" ] && [[ ! "$fw" == *.prl ]] && [[ ! "$fw" == */Headers ]] && [[ ! "$fw" == */Resources ]]; then
        check_dependencies "$fw" || TOTAL_MISSING=$((TOTAL_MISSING + $?))
    fi
done

# Check all libraries
for lib in "$FRAMEWORKS_DIR"/*.dylib; do
    if [ -f "$lib" ]; then
        check_dependencies "$lib" || TOTAL_MISSING=$((TOTAL_MISSING + $?))
    fi
done

# Check all plugins
for plugin in "$PLUGINS_DIR"/*/*.dylib; do
    if [ -f "$plugin" ]; then
        check_dependencies "$plugin" || TOTAL_MISSING=$((TOTAL_MISSING + $?))
    fi
done

if [ $TOTAL_MISSING -eq 0 ]; then
    status "All dependencies resolved!"
else
    error "Found $TOTAL_MISSING missing or incorrect dependencies"
    echo ""
    echo "Bundle may not work correctly on other machines!"
fi

# Code signing
echo ""
echo "Code signing application..."
codesign --force --deep --sign - "$BUILD_DIR/LocalVTT.app" 2>/dev/null || {
    warning "Code signing failed (non-critical for CI)"
}

echo ""
echo "================================================"
echo "Build Complete!"
echo "================================================"
echo "App Bundle: $BUILD_DIR/LocalVTT.app"
echo "Frameworks: $(ls -1 $FRAMEWORKS_DIR/*.framework 2>/dev/null | wc -l | xargs) Qt frameworks bundled"
echo "Libraries: $(ls -1 $FRAMEWORKS_DIR/*.dylib 2>/dev/null | wc -l | xargs) dependent libraries bundled"
echo "Plugins: $(find $PLUGINS_DIR -name '*.dylib' 2>/dev/null | wc -l | xargs) Qt plugins bundled"
echo ""

# Final system dependency check
echo "Final verification - checking for system Qt dependencies:"
if otool -L "$EXECUTABLE" | grep -E "/opt/homebrew|/usr/local/opt|@@HOMEBREW" > /dev/null; then
    error "WARNING: Executable still contains system Qt paths!"
    otool -L "$EXECUTABLE" | grep -E "/opt/homebrew|/usr/local/opt|@@HOMEBREW"
else
    status "✓ No system Qt dependencies found in executable"
fi

echo ""
echo "✅ CI build complete!"