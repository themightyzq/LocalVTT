#ifndef CUSTOMCURSORS_H
#define CUSTOMCURSORS_H

#include <QCursor>
#include <QPixmap>
#include <QSize>
#include <QColor>
#include "utils/ToolType.h"
#include "utils/FogToolMode.h"

class CustomCursors
{
public:
    // Main cursor creation method for tools
    static QCursor createCursorForTool(ToolType tool, bool isDragging = false);
    
    // Fog brush cursors with enhanced visual feedback
    static QCursor createFogBrushCursor(int brushSize, FogToolMode mode);
    
    // Pan tool cursors (open hand / closed hand)
    static QCursor createPanCursor(bool isDragging = false);
    
    // Measurement tool cursor with ruler aesthetic
    static QCursor createMeasurementCursor();
    
    // Pointer/beacon cursor for player attention
    static QCursor createPointerCursor();
    
    // Rectangle selection cursor for fog rectangle modes
    static QCursor createRectangleCursor();

private:
    // Helper functions for cursor creation
    static QPixmap createCircularPixmap(int diameter, const QColor& color, const QColor& outline = Qt::black);
    static QPixmap createCrosshairPixmap(int size = 32);
    static QPixmap createPlacementPixmap(int size = 24);
    static QPixmap createHandPixmap(bool isOpen = true, int size = 32);
    static QPixmap createRulerPixmap(int size = 32);
    static QPixmap createBeaconPixmap(int size = 32);
    
    // Color helpers for different fog modes
    static QColor getFogModeColor(FogToolMode mode, bool isOutline = false);
};

#endif // CUSTOMCURSORS_H