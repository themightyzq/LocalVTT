#ifndef VTTLOADER_H
#define VTTLOADER_H

#include <QImage>
#include <QColor>
#include <QString>
#include <QList>
#include <QPointF>
#include <QLineF>
#include <functional>

class VTTLoader
{
public:
    struct LightSource {
        QPointF position;
        qreal dimRadius;
        qreal brightRadius;
        QColor tintColor;
        qreal tintAlpha;
        qreal intensity;  // DD2VTT intensity value (typically 1.0 - 10.0)

        LightSource() : dimRadius(0), brightRadius(0), tintAlpha(0.0), intensity(1.0) {}
    };

    struct WallSegment {
        QLineF line;

        WallSegment() = default;
        WallSegment(const QPointF& start, const QPointF& end) : line(start, end) {}
        WallSegment(const QLineF& l) : line(l) {}
    };

    struct PortalData {
        QPointF position;
        QPointF bound1;
        QPointF bound2;
        qreal rotation;
        bool closed;
        bool freestanding;

        PortalData() : rotation(0.0), closed(false), freestanding(false) {}
        PortalData(const QPointF& pos, const QPointF& b1, const QPointF& b2,
                   qreal rot = 0.0, bool cls = false, bool fs = false)
            : position(pos), bound1(b1), bound2(b2), rotation(rot), closed(cls), freestanding(fs) {}
    };

    struct VTTData {
        QImage mapImage;
        int gridSquaresX;
        int gridSquaresY;
        int pixelsPerGrid;
        QColor ambientLight;
        bool isValid;
        QString errorMessage;

        // Foundry lighting data
        bool globalLight;
        qreal darkness;
        QList<LightSource> lights;

        // Wall data for line of sight
        QList<WallSegment> walls;

        // Portal data for doors/windows
        QList<PortalData> portals;

        VTTData() : gridSquaresX(0), gridSquaresY(0), pixelsPerGrid(50),
                    ambientLight(Qt::white), isValid(false), globalLight(true), darkness(0.0) {}
    };

    using ProgressCallback = std::function<void(int, const QString&)>;

    static VTTData loadVTT(const QString& filepath, ProgressCallback progressCallback = nullptr);
    static bool isVTTFile(const QString& filepath);

private:
    static QImage decodeBase64Image(const QString& base64Data, ProgressCallback progressCallback = nullptr);
    static QColor parseHexColor(const QString& hexColor);
};

#endif // VTTLOADER_H