#ifndef SCENEBUILDER_H
#define SCENEBUILDER_H

#include <QGraphicsScene>
#include <QImage>
#include <QByteArray>
#include <functional>

class QGraphicsPixmapItem;
class QGraphicsEllipseItem;
class GridOverlay;
class FogOfWar;

struct SceneConfig {
    bool gridEnabled = true;
    bool fogEnabled = false;
    int vttGridSize = 0;
    QByteArray fogState;
    std::function<void(const QRectF&)> fogChangeCallback;
};

struct SceneContents {
    QGraphicsPixmapItem* mapItem = nullptr;
    GridOverlay* gridOverlay = nullptr;
    FogOfWar* fogOverlay = nullptr;
    QGraphicsEllipseItem* fogBrushPreview = nullptr;
};

class SceneBuilder {
public:
    // Build all overlays in the given scene for the given map image.
    // Clears the scene first, then populates it.
    // Returns pointers to the created items (all owned by the scene).
    static SceneContents buildScene(
        QGraphicsScene* scene,
        const QImage& mapImage,
        const SceneConfig& config
    );

private:
    SceneBuilder() = default;
};

#endif // SCENEBUILDER_H
