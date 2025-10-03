#include "SceneManager.h"
#include "GridOverlay.h"
#include "FogOfWar.h"
#include "WallSystem.h"
#include "PortalSystem.h"
#include "LightingOverlay.h"
#include "utils/DebugConsole.h"
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QMutexLocker>

QRecursiveMutex SceneManager::s_sceneMutex;

SceneManager::SceneManager(QObject* parent)
    : QObject(parent)
    , m_scene(nullptr)
    , m_mapItem(nullptr)
    , m_gridOverlay(nullptr)
    , m_fogOverlay(nullptr)
    , m_wallSystem(nullptr)
    , m_portalSystem(nullptr)
    , m_lightingOverlay(nullptr)
    , m_gridEnabled(true)
    , m_fogEnabled(true)
    , m_wallsEnabled(false)
    , m_portalsEnabled(false)
    , m_ownScene(true)
    , m_vttGridSize(0)
    , m_showParsedLights(false)
{
    m_scene = new QGraphicsScene(this);
}

SceneManager::~SceneManager()
{
    clearScene();
}

void SceneManager::clearScene()
{
    if (!m_scene || !m_ownScene) return;

    QMutexLocker locker(&s_sceneMutex);

    m_scene->clear();

    m_mapItem = nullptr;
    m_gridOverlay = nullptr;
    m_fogOverlay = nullptr;
    m_wallSystem = nullptr;
    m_portalSystem = nullptr;
    m_lightingOverlay = nullptr;

    m_lightDebugItems.clear();
}

bool SceneManager::setupNewMap(const QImage& image, int vttGridSize)
{
    if (image.isNull()) return false;

    clearScene();

    QMutexLocker locker(&s_sceneMutex);

    m_vttGridSize = vttGridSize;

    m_mapItem = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    m_mapItem->setZValue(0);
    m_scene->addItem(m_mapItem);
    m_scene->setSceneRect(m_mapItem->boundingRect());

    recreateOverlays();

    DebugConsole::info(QString("Map loaded: %1x%2 pixels").arg(image.width()).arg(image.height()), "Graphics");

    return true;
}

void SceneManager::recreateOverlays()
{
    if (!m_mapItem) return;

    QRectF bounds = m_mapItem->boundingRect();

    m_gridOverlay = new GridOverlay();
    m_gridOverlay->setMapSize(bounds.size().toSize());
    if (m_vttGridSize > 0) {
        m_gridOverlay->setGridSize(m_vttGridSize);
    }
    m_scene->addItem(m_gridOverlay);
    m_gridOverlay->setZValue(1);
    updateGrid();

    m_wallSystem = new WallSystem();
    m_scene->addItem(m_wallSystem);
    m_wallSystem->setZValue(5);
    m_wallSystem->setVisible(m_wallsEnabled);

    m_portalSystem = new PortalSystem();
    m_scene->addItem(m_portalSystem);
    m_portalSystem->setZValue(10);
    m_portalSystem->setVisible(m_portalsEnabled);

    // Portal system doesn't need wall system reference

    m_fogOverlay = new FogOfWar();
    m_fogOverlay->setMapSize(bounds.size().toSize());
    m_fogOverlay->setWallSystem(m_wallSystem);
    m_scene->addItem(m_fogOverlay);
    m_fogOverlay->setZValue(2);
    // Fog changes are handled through the FogOfWar class directly
    updateFog();

}

void SceneManager::shareScene(SceneManager* sourceManager)
{
    if (!sourceManager || !sourceManager->m_scene) return;

    if (m_ownScene && m_scene) {
        delete m_scene;
    }

    m_scene = sourceManager->m_scene;
    m_mapItem = sourceManager->m_mapItem;
    m_gridOverlay = sourceManager->m_gridOverlay;
    m_fogOverlay = sourceManager->m_fogOverlay;
    m_wallSystem = sourceManager->m_wallSystem;
    m_portalSystem = sourceManager->m_portalSystem;
    m_lightingOverlay = sourceManager->m_lightingOverlay;
    m_ownScene = false;
}

void SceneManager::updateSharedScene()
{
    if (!m_scene || m_ownScene) return;

    QMutexLocker locker(&s_sceneMutex);
    m_scene->update();
}

void SceneManager::setGridEnabled(bool enabled)
{
    m_gridEnabled = enabled;
    updateGrid();
}

void SceneManager::setFogEnabled(bool enabled)
{
    const bool stateChanged = (m_fogEnabled != enabled);
    m_fogEnabled = enabled;
    updateFog();

    if (stateChanged) {
        emit fogChanged();
    }
}


void SceneManager::setWallsEnabled(bool enabled)
{
    m_wallsEnabled = enabled;
    if (m_wallSystem) {
        QMutexLocker locker(&s_sceneMutex);
        if (enabled) {
            if (!m_scene->items().contains(m_wallSystem)) {
                m_scene->addItem(m_wallSystem);
                m_wallSystem->setZValue(5);
            }
            m_wallSystem->setVisible(true);
        } else {
            m_wallSystem->setVisible(false);
        }
    }
}

void SceneManager::setPortalsEnabled(bool enabled)
{
    m_portalsEnabled = enabled;
    if (m_portalSystem) {
        QMutexLocker locker(&s_sceneMutex);
        if (enabled) {
            if (!m_scene->items().contains(m_portalSystem)) {
                m_scene->addItem(m_portalSystem);
                m_portalSystem->setZValue(10);
            }
            m_portalSystem->setVisible(true);
        } else {
            m_portalSystem->setVisible(false);
        }
    }
}

void SceneManager::setLightingEnabled(bool enabled)
{
    if (enabled && !m_lightingOverlay && m_mapItem) {
        m_lightingOverlay = new LightingOverlay();
        QMutexLocker locker(&s_sceneMutex);
        m_scene->addItem(m_lightingOverlay);
        m_lightingOverlay->setZValue(4);
    }

    if (m_lightingOverlay) {
        m_lightingOverlay->setVisible(enabled);
    }
}

bool SceneManager::isLightingEnabled() const
{
    return m_lightingOverlay && m_lightingOverlay->isVisible();
}

LightingOverlay* SceneManager::getLightingOverlay()
{
    if (!m_lightingOverlay && m_mapItem) {
        m_lightingOverlay = new LightingOverlay();
        QMutexLocker locker(&s_sceneMutex);
        m_scene->addItem(m_lightingOverlay);
        m_lightingOverlay->setZValue(4);
    }
    return m_lightingOverlay;
}

void SceneManager::setParsedLights(const QList<VTTLoader::LightSource>& lights)
{
    m_parsedLights = lights;
    if (m_showParsedLights) {
        updateParsedLightOverlays();
    }
}

void SceneManager::setShowParsedLights(bool enabled)
{
    m_showParsedLights = enabled;
    updateParsedLightOverlays();
}

void SceneManager::updateGrid()
{
    if (!m_gridOverlay || !m_scene) return;

    QMutexLocker locker(&s_sceneMutex);

    if (m_gridEnabled) {
        if (!m_scene->items().contains(m_gridOverlay)) {
            m_scene->addItem(m_gridOverlay);
            m_gridOverlay->setZValue(1);
        }
        m_gridOverlay->setVisible(true);
    } else {
        m_gridOverlay->setVisible(false);
    }
}

void SceneManager::updateFog()
{
    if (!m_fogOverlay || !m_scene) return;

    QMutexLocker locker(&s_sceneMutex);

    if (m_fogEnabled) {
        if (!m_scene->items().contains(m_fogOverlay)) {
            m_scene->addItem(m_fogOverlay);
            m_fogOverlay->setZValue(2);
        }
        m_fogOverlay->setVisible(true);
    } else {
        m_fogOverlay->setVisible(false);
    }
}


void SceneManager::updateParsedLightOverlays()
{
    QMutexLocker locker(&s_sceneMutex);

    for (auto* item : m_lightDebugItems) {
        m_scene->removeItem(item);
        delete item;
    }
    m_lightDebugItems.clear();

    if (m_showParsedLights && !m_parsedLights.isEmpty()) {
        for (const auto& light : m_parsedLights) {
            // Use dimRadius as the visual indicator for light range
            qreal range = light.dimRadius > 0 ? light.dimRadius : light.brightRadius;
            auto* ellipse = new QGraphicsEllipseItem(
                light.position.x() - range,
                light.position.y() - range,
                range * 2,
                range * 2
            );

            ellipse->setPen(QPen(light.tintColor, 2, Qt::DashLine));
            ellipse->setBrush(QBrush(QColor(light.tintColor.red(), light.tintColor.green(),
                                             light.tintColor.blue(), 30)));
            ellipse->setZValue(20);

            m_scene->addItem(ellipse);
            m_lightDebugItems.append(ellipse);
        }

        DebugConsole::info(QString("Showing %1 parsed VTT light sources").arg(m_parsedLights.size()), "VTT");
    }
}

QByteArray SceneManager::saveFogState() const
{
    if (m_fogOverlay) {
        return m_fogOverlay->saveState();
    }
    return QByteArray();
}

bool SceneManager::loadFogState(const QByteArray& data)
{
    if (m_fogOverlay) {
        return m_fogOverlay->loadState(data);
    }
    return false;
}

