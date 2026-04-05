#!/bin/bash
set -euo pipefail

# Crit VTT — Release Build Script
# Usage: ./scripts/build-release.sh [--sign "Developer ID Application: Name (TEAMID)"]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build-release"

SIGN_IDENTITY=""
while [[ $# -gt 0 ]]; do
    case $1 in
        --sign) SIGN_IDENTITY="$2"; shift 2;;
        *) echo "Unknown option: $1"; exit 1;;
    esac
done

echo "=== Crit VTT Release Build ==="
echo "Project: $PROJECT_DIR"
echo "Build:   $BUILD_DIR"
echo "Sign:    ${SIGN_IDENTITY:-unsigned}"
echo ""

# Step 1: Clean build
echo "--- Step 1: Clean build ---"
rm -rf "$BUILD_DIR"
cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -- -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

APP="$BUILD_DIR/CritVTT.app"
if [ ! -d "$APP" ]; then
    echo "ERROR: Build failed"
    exit 1
fi

# Step 2: Deploy Qt
echo "--- Step 2: macdeployqt ---"
MACDEPLOYQT=$(which macdeployqt 2>/dev/null || echo "")
if [ -z "$MACDEPLOYQT" ]; then
    MACDEPLOYQT=$(find /opt/homebrew -name "macdeployqt" -print -quit 2>/dev/null || echo "")
fi
if [ -z "$MACDEPLOYQT" ]; then
    echo "ERROR: macdeployqt not found"
    exit 1
fi
"$MACDEPLOYQT" "$APP" -always-overwrite

# Step 3: Re-sign (macdeployqt invalidates the signature)
echo "--- Step 3: Code signing ---"
if [ -n "$SIGN_IDENTITY" ]; then
    codesign --deep --force --options runtime --sign "$SIGN_IDENTITY" "$APP"
else
    codesign --deep --force --sign - "$APP"
fi

# Step 4: Verify
echo "--- Step 4: Verify ---"
MISSING=0
for plugin in "PlugIns/platforms/libqcocoa.dylib" "PlugIns/imageformats/libqjpeg.dylib" "PlugIns/imageformats/libqsvg.dylib" "PlugIns/imageformats/libqwebp.dylib"; do
    if [ ! -f "$APP/Contents/$plugin" ]; then
        echo "MISSING: $plugin"
        MISSING=1
    fi
done

EXTERNAL=$(otool -L "$APP/Contents/MacOS/CritVTT" | grep -v "@rpath\|@executable_path\|/System\|/usr/lib\|:$" || true)
if [ -n "$EXTERNAL" ]; then
    echo "WARNING: External dependencies: $EXTERNAL"
fi

# Step 5: Create DMG
echo "--- Step 5: Create DMG ---"
VERSION=$(grep "project(CritVTT VERSION" "$PROJECT_DIR/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
DMG_NAME="CritVTT-${VERSION:-1.0.0}-macOS"
rm -f "$BUILD_DIR/${DMG_NAME}.dmg"

DMG_STAGING=$(mktemp -d)
cp -R "$APP" "$DMG_STAGING/"
ln -s /Applications "$DMG_STAGING/Applications"
hdiutil create -volname "Crit VTT" -srcfolder "$DMG_STAGING" -ov -format UDZO "$BUILD_DIR/${DMG_NAME}.dmg"
rm -rf "$DMG_STAGING"

echo ""
echo "=== Release Build Complete ==="
echo "App: $APP"
echo "DMG: $BUILD_DIR/${DMG_NAME}.dmg"
echo "Size: $(du -sh "$BUILD_DIR/${DMG_NAME}.dmg" | cut -f1)"
