#include "utils/CustomCursors.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPainterPath>
#include <cmath>

QCursor CustomCursors::createCursorForTool(ToolType tool, bool isDragging)
{
    switch (tool) {
        case ToolType::Pointer:
            return createPointerCursor();
        case ToolType::FogBrush:
            return createFogBrushCursor(60, FogToolMode::UnifiedFog);
        case ToolType::FogRectangle:
            return createRectangleCursor();
        default:
            return QCursor(Qt::ArrowCursor);
    }
}

QCursor CustomCursors::createFogBrushCursor(int brushSize, FogToolMode mode)
{
    // Make the cursor visually represent the actual brush size
    // Clamp to a reasonable OS-supported maximum to avoid giant cursors
    int cursorSize = qBound(8, brushSize, 256);

    // Create pixmap with some padding for the outline
    int pixmapSize = cursorSize + 4;
    QPixmap pixmap(pixmapSize, pixmapSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Calculate circle position and size
    int circleSize = cursorSize;
    int x = (pixmapSize - circleSize) / 2;
    int y = (pixmapSize - circleSize) / 2;
    QRect circleRect(x, y, circleSize, circleSize);

    // Set colors based on fog mode
    QColor fillColor = getFogModeColor(mode, false);
    QColor outlineColor = getFogModeColor(mode, true);

    // Draw the brush preview circle
    painter.setPen(QPen(outlineColor, 2));
    painter.setBrush(QBrush(fillColor));
    painter.drawEllipse(circleRect);

    // Add a center crosshair for precision
    painter.setPen(QPen(Qt::white, 1));
    int centerX = pixmapSize / 2;
    int centerY = pixmapSize / 2;
    painter.drawLine(centerX - 4, centerY, centerX + 4, centerY);
    painter.drawLine(centerX, centerY - 4, centerX, centerY + 4);

    painter.end();

    // Set hotspot to center of cursor
    QPoint hotspot(pixmapSize / 2, pixmapSize / 2);
    return QCursor(pixmap, hotspot.x(), hotspot.y());
}

QCursor CustomCursors::createPanCursor(bool isDragging)
{
    if (isDragging) {
        return QCursor(Qt::ClosedHandCursor);
    } else {
        return QCursor(Qt::OpenHandCursor);
    }
}

QCursor CustomCursors::createMeasurementCursor()
{
    int size = 32;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw crosshair lines
    painter.setPen(QPen(Qt::white, 2));
    int center = size / 2;

    // Horizontal line
    painter.drawLine(4, center, size - 4, center);
    // Vertical line
    painter.drawLine(center, 4, center, size - 4);

    // Add small measuring marks
    painter.setPen(QPen(Qt::yellow, 1));
    for (int i = 8; i < size; i += 8) {
        // Horizontal marks
        painter.drawLine(i, center - 2, i, center + 2);
        // Vertical marks
        painter.drawLine(center - 2, i, center + 2, i);
    }

    // Add center dot for precision
    painter.setPen(QPen(Qt::red, 1));
    painter.setBrush(QBrush(Qt::red));
    painter.drawEllipse(center - 1, center - 1, 2, 2);

    painter.end();

    return QCursor(pixmap, center, center);
}

QCursor CustomCursors::createPointerCursor()
{
    QPixmap pixmap = createBeaconPixmap(32);
    return QCursor(pixmap, 16, 16);
}

QCursor CustomCursors::createRectangleCursor()
{
    int size = 32;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int center = size / 2;

    // Draw crosshair
    painter.setPen(QPen(Qt::white, 1));
    painter.drawLine(4, center, size - 4, center);
    painter.drawLine(center, 4, center, size - 4);

    // Draw rectangle corners to indicate rectangle selection mode
    painter.setPen(QPen(Qt::yellow, 2));
    int cornerSize = 6;
    int offset = 8;

    // Top-left corner
    painter.drawLine(center - offset, center - offset, center - offset + cornerSize, center - offset);
    painter.drawLine(center - offset, center - offset, center - offset, center - offset + cornerSize);

    // Top-right corner
    painter.drawLine(center + offset, center - offset, center + offset - cornerSize, center - offset);
    painter.drawLine(center + offset, center - offset, center + offset, center - offset + cornerSize);

    // Bottom-left corner
    painter.drawLine(center - offset, center + offset, center - offset + cornerSize, center + offset);
    painter.drawLine(center - offset, center + offset, center - offset, center + offset - cornerSize);

    // Bottom-right corner
    painter.drawLine(center + offset, center + offset, center + offset - cornerSize, center + offset);
    painter.drawLine(center + offset, center + offset, center + offset, center + offset - cornerSize);

    painter.end();

    return QCursor(pixmap, center, center);
}

QPixmap CustomCursors::createCircularPixmap(int diameter, const QColor& color, const QColor& outline)
{
    QPixmap pixmap(diameter + 4, diameter + 4);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int x = 2;
    int y = 2;
    QRect rect(x, y, diameter, diameter);

    painter.setPen(QPen(outline, 2));
    painter.setBrush(QBrush(color));
    painter.drawEllipse(rect);

    painter.end();

    return pixmap;
}

QPixmap CustomCursors::createHandPixmap(bool isOpen, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    int center = size / 2;
    
    if (isOpen) {
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(QBrush(QColor(0, 0, 0, 100)));
        
        QPainterPath hand;
        hand.moveTo(center - 6, center + 8);
        hand.lineTo(center - 8, center - 2);
        hand.lineTo(center - 4, center - 8);
        hand.lineTo(center - 2, center - 6);
        hand.lineTo(center, center - 8);
        hand.lineTo(center + 2, center - 6);
        hand.lineTo(center + 4, center - 8);
        hand.lineTo(center + 8, center - 2);
        hand.lineTo(center + 6, center + 8);
        hand.closeSubpath();
        
        painter.drawPath(hand);
    } else {
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(QBrush(QColor(0, 0, 0, 100)));
        
        QPainterPath closedHand;
        closedHand.addEllipse(center - 6, center - 4, 12, 8);
        painter.drawPath(closedHand);
        
        for (int i = 0; i < 4; i++) {
            int x = center - 4 + i * 2;
            painter.drawLine(x, center - 2, x, center + 2);
        }
    }
    
    painter.end();
    return pixmap;
}

QPixmap CustomCursors::createRulerPixmap(int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    int center = size / 2;
    
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(4, center, size - 4, center);
    painter.drawLine(center, 4, center, size - 4);
    
    painter.setPen(QPen(Qt::yellow, 1));
    for (int i = 8; i < size; i += 4) {
        painter.drawLine(i, center - 1, i, center + 1);
        painter.drawLine(center - 1, i, center + 1, i);
    }
    
    painter.setPen(QPen(Qt::red, 1));
    painter.setBrush(QBrush(Qt::red));
    painter.drawEllipse(center - 1, center - 1, 2, 2);
    
    painter.end();
    return pixmap;
}

QPixmap CustomCursors::createBeaconPixmap(int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    int center = size / 2;
    int radius = size / 3;
    
    QRadialGradient gradient(center, center, radius);
    gradient.setColorAt(0, QColor(255, 255, 0, 200));
    gradient.setColorAt(1, QColor(255, 100, 0, 100));
    
    painter.setBrush(QBrush(gradient));
    painter.setPen(QPen(Qt::white, 2));
    painter.drawEllipse(center - radius, center - radius, radius * 2, radius * 2);
    
    painter.setPen(QPen(Qt::white, 1));
    painter.drawLine(center, center - radius - 4, center, center - radius + 2);
    painter.drawLine(center, center + radius - 2, center, center + radius + 4);
    painter.drawLine(center - radius - 4, center, center - radius + 2, center);
    painter.drawLine(center + radius - 2, center, center + radius + 4, center);
    
    painter.end();
    return pixmap;
}

QColor CustomCursors::getFogModeColor(FogToolMode mode, bool isOutline)
{
    switch (mode) {
        case FogToolMode::UnifiedFog:
            // Unified fog tool uses green for reveal operations by default
            // The actual behavior (reveal vs hide) is determined at runtime
            return isOutline ? QColor(0, 255, 0) : QColor(100, 255, 100, 80);

        case FogToolMode::DrawPen:
            // Drawing pen mode - white/transparent
            return isOutline ? Qt::white : QColor(255, 255, 255, 80);

        case FogToolMode::DrawEraser:
            // Eraser mode - red for removal
            return isOutline ? QColor(255, 0, 0) : QColor(255, 100, 100, 80);

        default:
            return isOutline ? Qt::white : QColor(255, 255, 255, 80);
    }
}

QPixmap CustomCursors::createCrosshairPixmap(int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QPen(Qt::white, 1));
    int center = size / 2;

    painter.drawLine(0, center, size, center);
    painter.drawLine(center, 0, center, size);

    painter.end();

    return pixmap;
}

QPixmap CustomCursors::createPlacementPixmap(int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int center = size / 2;
    int radius = size / 2 - 2;

    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(QBrush(QColor(0, 150, 255, 100)));
    painter.drawEllipse(center - radius, center - radius, radius * 2, radius * 2);

    painter.setPen(QPen(Qt::white, 1));
    painter.drawLine(center - 3, center, center + 3, center);
    painter.drawLine(center, center - 3, center, center + 3);

    painter.end();

    return pixmap;
}
