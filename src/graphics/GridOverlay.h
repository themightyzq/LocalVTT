#ifndef GRIDOVERLAY_H
#define GRIDOVERLAY_H

#include <QGraphicsItem>
#include <QSize>

class GridOverlay : public QGraphicsItem
{
public:
    GridOverlay();

    void setMapSize(const QSize& size);
    void setGridSize(int size);
    void setGridColor(const QColor& color);
    void setGridOpacity(qreal opacity);

    // D&D measurement support
    void setFeetPerSquare(double feet);
    void setPixelsPerInch(double ppi);

    int getGridSize() const { return m_gridSize; }
    double getFeetPerSquare() const { return m_feetPerSquare; }
    double getPixelsPerInch() const { return m_pixelsPerInch; }
    double getGameScaleInches() const;
    QString getGridInfo() const;

    // Calculate D&D standard grid size based on DPI
    static int calculateDnDGridSize(double screenDpi = 96.0);

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    void paintSquareGrid(QPainter* painter);

    QSize m_mapSize;
    int m_gridSize;
    QColor m_gridColor;
    qreal m_gridOpacity;

    // D&D measurement properties
    double m_feetPerSquare;     // Game distance per square (default: 5 feet)
    double m_pixelsPerInch;     // Screen pixels per inch (for scale calculation)
};

#endif // GRIDOVERLAY_H