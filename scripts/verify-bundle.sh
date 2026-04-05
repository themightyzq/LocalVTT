#!/bin/bash
set -euo pipefail

# Crit VTT — Bundle Verification
# Usage: ./scripts/verify-bundle.sh path/to/CritVTT.app

APP="${1:?Usage: verify-bundle.sh path/to/CritVTT.app}"

if [ ! -d "$APP" ]; then
    echo "ERROR: $APP not found"
    exit 1
fi

echo "=== Verifying: $APP ==="
ERRORS=0

BINARY="$APP/Contents/MacOS/CritVTT"
if [ ! -f "$BINARY" ]; then
    echo "FAIL: Binary not found"; exit 1
fi
echo "OK: Binary exists"

PLIST="$APP/Contents/Info.plist"
if [ -f "$PLIST" ]; then
    VERSION=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "$PLIST" 2>/dev/null || echo "?")
    BUNDLE_ID=$(/usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "$PLIST" 2>/dev/null || echo "?")
    echo "OK: Info.plist — Version=$VERSION, ID=$BUNDLE_ID"
else
    echo "FAIL: Info.plist missing"; ERRORS=$((ERRORS + 1))
fi

echo "--- Plugins ---"
for entry in \
    "PlugIns/platforms/libqcocoa.dylib:App launch" \
    "PlugIns/imageformats/libqjpeg.dylib:JPG maps" \
    "PlugIns/imageformats/libqsvg.dylib:SVG icons" \
    "PlugIns/imageformats/libqwebp.dylib:WebP maps"; do
    PLUGIN="${entry%%:*}"; PURPOSE="${entry##*:}"
    if [ -f "$APP/Contents/$PLUGIN" ]; then
        echo "  OK: $PLUGIN ($PURPOSE)"
    else
        echo "  FAIL: $PLUGIN MISSING — $PURPOSE broken"; ERRORS=$((ERRORS + 1))
    fi
done

echo "--- Frameworks ---"
FC=$(ls "$APP/Contents/Frameworks/" 2>/dev/null | wc -l | tr -d ' ')
echo "  Bundled: $FC frameworks"
[ "$FC" -lt 5 ] && echo "  WARNING: Too few frameworks" && ERRORS=$((ERRORS + 1))

echo "--- External Dependencies ---"
EXT=$(otool -L "$BINARY" | grep -v "@rpath\|@executable_path\|/System\|/usr/lib\|:$" || true)
if [ -n "$EXT" ]; then
    echo "  FAIL: $EXT"; ERRORS=$((ERRORS + 1))
else
    echo "  OK: Self-contained"
fi

echo "--- qt.conf ---"
[ -f "$APP/Contents/Resources/qt.conf" ] && echo "  OK" || { echo "  FAIL: missing"; ERRORS=$((ERRORS + 1)); }

echo "--- Code Signing ---"
codesign --verify --deep --strict "$APP" 2>/dev/null && echo "  OK: Signed" || echo "  WARNING: Not signed or invalid"

echo ""
if [ $ERRORS -eq 0 ]; then
    echo "ALL CHECKS PASSED"
else
    echo "$ERRORS ISSUE(S) FOUND"
fi
exit $ERRORS
