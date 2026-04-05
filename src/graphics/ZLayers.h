#ifndef ZLAYERS_H
#define ZLAYERS_H

// Canonical Z-layer ordering for QGraphicsScene items.
// Source of truth — CLAUDE.md §4.1 defers here.
// All setZValue() calls MUST use these constants.
namespace ZLayer {
    constexpr qreal Map             = 0.0;
    constexpr qreal Grid            = 10.0;
    constexpr qreal Fog             = 20.0;
    constexpr qreal Beacons         = 30.0;
    constexpr qreal PointLights     = 35.0;
    constexpr qreal FogMist         = 40.0;
    constexpr qreal Weather         = 50.0;
    constexpr qreal Lightning       = 70.0;
    constexpr qreal ToolPreview     = 100.0;
    constexpr qreal LightingOverlay = 600.0;
    constexpr qreal BrushPreview    = 1000.0;
}

#endif // ZLAYERS_H
