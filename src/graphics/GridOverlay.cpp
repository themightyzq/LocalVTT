#include "graphics/GridOverlay.h"
#include <QPainter>
#include <QPen>
#include <QApplication>
#include <QScreen>

GridOverlay::GridOverlay()
    : m_gridSize(150)  // Default grid size: 150px
    , m_gridColor(Qt::white)
    , m_gridOpacity(0.3)
    , m_feetPerSquare(5.0)  // D&D standard: 5 feet per square
    , m_pixelsPerInch(96.0)  // Default screen DPI
{
    setFlag(QGraphicsItem::ItemIgnoresTransformations, false);

    // Get actual screen DPI for reference
    if (QApplication::primaryScreen()) {
        m_pixelsPerInch = QApplication::primaryScreen()->logicalDotsPerInch();
    }
}

void GridOverlay::setMapSize(const QSize& size)
{
    m_mapSize = size;
    prepareGeometryChange();
}

void GridOverlay::setGridSize(int size)
{
    m_gridSize = size;
    // Recalculate scale when grid size changes
    update();
}

void GridOverlay::setGridColor(const QColor& color)
{
    m_gridColor = color;
    update();
}

void GridOverlay::setGridOpacity(qreal opacity)
{
    m_gridOpacity = opacity;
    update();
}


QRectF GridOverlay::boundingRect() const
{
    return QRectF(0, 0, m_mapSize.width(), m_mapSize.height());
}

void GridOverlay::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (m_mapSize.isEmpty() || m_gridSize <= 0) {
        return;
    }

    // Set up pen for grid lines
    QPen pen(m_gridColor);
    pen.setWidth(1);
    pen.setCosmetic(true);  // Keep line width constant regardless of zoom
    painter->setPen(pen);
    painter->setOpacity(m_gridOpacity);

    // Draw square grid
    paintSquareGrid(painter);
}

void GridOverlay::setFeetPerSquare(double feet)
{
    m_feetPerSquare = feet;
    update();
}

void GridOverlay::setPixelsPerInch(double ppi)
{
    m_pixelsPerInch = ppi;
    update();
}

double GridOverlay::getGameScaleInches() const
{
    // Convert grid size in pixels to inches on screen
    return static_cast<double>(m_gridSize) / m_pixelsPerInch;
}

QString GridOverlay::getGridInfo() const
{
    double scaleInches = getGameScaleInches();
    QString gridTypeName = "Square"; // Only square grids supported
    return QString("%1 Grid: %2px = %3 ft (%4\" on screen @ %5 DPI)")
           .arg(gridTypeName)
           .arg(m_gridSize)
           .arg(m_feetPerSquare, 'f', 1)
           .arg(scaleInches, 'f', 2)
           .arg(m_pixelsPerInch, 'f', 0);
}

void GridOverlay::paintSquareGrid(QPainter* painter)
{
    // Draw vertical lines
    for (int x = 0; x <= m_mapSize.width(); x += m_gridSize) {
        painter->drawLine(x, 0, x, m_mapSize.height());
    }

    // Draw horizontal lines
    for (int y = 0; y <= m_mapSize.height(); y += m_gridSize) {
        painter->drawLine(0, y, m_mapSize.width(), y);
    }
}

int GridOverlay::calculateDnDGridSize(double screenDpi)
{
    // D&D standard: 1 inch on paper/screen = 5 feet in game
    // For typical screen viewing, we want squares to be visible but not too large
    // Target: 1 inch squares at actual DPI, with wider range for flexibility

    int idealSize = static_cast<int>(screenDpi);  // 1 inch worth of pixels

    // Clamp to reasonable range for screen viewing (20-200 pixels)
    if (idealSize < 20) {
        return 20;  // Minimum readable size
    } else if (idealSize > 200) {
        return 96;  // Good compromise for high-DPI screens
    }

    return idealSize;
}