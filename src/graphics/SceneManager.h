#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <QObject>
#include <QRecursiveMutex>
#include <QImage>
#include "utils/VTTLoader.h"

class QGraphicsScene;
class QGraphicsPixmapItem;
class GridOverlay;
class FogOfWar;
class WallSystem;
class PortalSystem;
class LightingOverlay;
class QGraphicsEllipseItem;

class SceneManager : public QObject
{
    Q_OBJECT

public:
    explicit SceneManager(QObject* parent = nullptr);
    ~SceneManager();

    QGraphicsScene* getScene() const { return m_scene; }
    QGraphicsPixmapItem* getMapItem() const { return m_mapItem; }
    GridOverlay* getGridOverlay() const { return m_gridOverlay; }
    FogOfWar* getFogOverlay() const { return m_fogOverlay; }
    WallSystem* getWallSystem() const { return m_wallSystem; }
    PortalSystem* getPortalSystem() const { return m_portalSystem; }
    LightingOverlay* getLightingOverlay();

    void clearScene();
    bool setupNewMap(const QImage& image, int vttGridSize = 0);
    void shareScene(SceneManager* sourceManager);
    void updateSharedScene();

    void setGridEnabled(bool enabled);
    void setFogEnabled(bool enabled);
    void setWallsEnabled(bool enabled);
    void setPortalsEnabled(bool enabled);
    void setLightingEnabled(bool enabled);

    bool isGridEnabled() const { return m_gridEnabled; }
    bool isFogEnabled() const { return m_fogEnabled; }
    bool areWallsEnabled() const { return m_wallsEnabled; }
    bool arePortalsEnabled() const { return m_portalsEnabled; }
    bool isLightingEnabled() const;

    void setParsedLights(const QList<VTTLoader::LightSource>& lights);
    void setShowParsedLights(bool enabled);

    QByteArray saveFogState() const;
    bool loadFogState(const QByteArray& data);

    static QRecursiveMutex& getSceneMutex() { return s_sceneMutex; }

signals:
    void fogChanged();

private:
    void updateGrid();
    void updateFog();
    void updateParsedLightOverlays();
    void recreateOverlays();

    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_mapItem;
    GridOverlay* m_gridOverlay;
    FogOfWar* m_fogOverlay;
    WallSystem* m_wallSystem;
    PortalSystem* m_portalSystem;
    LightingOverlay* m_lightingOverlay;

    bool m_gridEnabled;
    bool m_fogEnabled;
    bool m_wallsEnabled;
    bool m_portalsEnabled;
    bool m_ownScene;
    int m_vttGridSize;

    QList<VTTLoader::LightSource> m_parsedLights;
    QList<QGraphicsEllipseItem*> m_lightDebugItems;
    bool m_showParsedLights;

    static QRecursiveMutex s_sceneMutex;
};

#endif // SCENEMANAGER_H