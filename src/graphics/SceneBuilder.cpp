#include "graphics/SceneBuilder.h"
#include "graphics/GridOverlay.h"
#include "graphics/FogOfWar.h"
#include "graphics/ZLayers.h"
#include "utils/DebugConsole.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QPixmap>

SceneContents SceneBuilder::buildScene(
    QGraphicsScene* scene,
    const QImage& mapImage,
    const SceneConfig& config)
{
    SceneContents contents;

    if (!scene || mapImage.isNull()) {
        DebugConsole::error("SceneBuilder: null scene or image", "Rendering");
        return contents;
    }

    // 1. Clear scene (deletes all items)
    scene->clear();

    // 2. Create map pixmap item
    QPixmap pixmap = QPixmap::fromImage(mapImage);
    contents.mapItem = scene->addPixmap(pixmap);
    if (!contents.mapItem) {
        DebugConsole::error("SceneBuilder: failed to create map item", "Rendering");
        return contents;
    }
    contents.mapItem->setZValue(ZLayer::Map);
    scene->setSceneRect(pixmap.rect());

    // 3. Create grid overlay
    contents.gridOverlay = new GridOverlay();
    contents.gridOverlay->setMapSize(pixmap.size());
    if (config.vttGridSize > 0) {
        contents.gridOverlay->setGridSize(config.vttGridSize);
    }
    if (config.gridEnabled) {
        scene->addItem(contents.gridOverlay);
        contents.gridOverlay->setZValue(ZLayer::Grid);
    }

    // 4. Create fog overlay
    contents.fogOverlay = new FogOfWar();
    contents.fogOverlay->setMapSize(pixmap.size());
    if (!config.fogState.isEmpty()) {
        contents.fogOverlay->loadState(config.fogState);
    }
    if (config.fogChangeCallback) {
        contents.fogOverlay->setChangeCallback(config.fogChangeCallback);
    }
    if (config.fogEnabled) {
        scene->addItem(contents.fogOverlay);
        contents.fogOverlay->setZValue(ZLayer::Fog);
    }

    // 5. Create fog brush preview (hidden by default)
    contents.fogBrushPreview = new QGraphicsEllipseItem();
    contents.fogBrushPreview->setVisible(false);
    contents.fogBrushPreview->setZValue(ZLayer::BrushPreview);
    scene->addItem(contents.fogBrushPreview);

    DebugConsole::info(QString("SceneBuilder: scene built (%1x%2)")
        .arg(pixmap.width()).arg(pixmap.height()), "Rendering");

    return contents;
}
