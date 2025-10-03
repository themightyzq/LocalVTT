#ifndef MAPDISPLAY_H
#define MAPDISPLAY_H

#include <QGraphicsView>
#include <QImage>
#include <QUuid>
#include <QRecursiveMutex>
#include <QMutexLocker>
#include <memory>
#include "utils/VTTLoader.h"
#include "utils/ToolType.h"

class OpenGLMapDisplay;

class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsRectItem;
class GridOverlay;
class FogOfWar;
class WallSystem;
class PortalSystem;
class PingIndicator;
class GMBeacon;
class LightingOverlay;
class MainWindow;
class ZoomIndicator;
class LoadingProgressWidget;
class ImageLoader;

// Forward declaration for fog tool mode
enum class FogToolMode;

class MapDisplay : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapDisplay(QWidget *parent = nullptr);
    ~MapDisplay();

    // Core functionality
    bool loadImage(const QString& path);
    bool loadImageWithProgress(const QString& path);
    bool loadImageFromCache(const QImage& cachedImage, const VTTLoader::VTTData& vttData);
    void setCachedImage(const QImage& image);
    void shareScene(MapDisplay* sourceDisplay);
    void updateSharedScene();  // CRITICAL FIX: Update shared displays safely

    // NEW: Direct map copying instead of scene sharing
    void copyMapFrom(MapDisplay* sourceDisplay);
    QImage getCurrentMapImage() const { return m_currentMap; }

    // Grid control
    void setGridEnabled(bool enabled);
    bool isGridEnabled() const { return m_gridEnabled; }

    // Fog of War control
    void setFogEnabled(bool enabled);
    bool isFogEnabled() const { return m_fogEnabled; }
    void notifyFogChanged(const QRectF& dirtyRegion = QRectF()); // PRIORITY 4 FIX: Notify with dirty region
    void clearFog();
    void resetFog();


    // Fog brush size control
    void setFogBrushSize(int size);
    int getFogBrushSize() const { return m_fogBrushSize; }

    // Fog hide mode control
    void setFogHideModeEnabled(bool enabled);
    bool isFogHideModeEnabled() const { return m_fogHideModeEnabled; }

    // Fog rectangle mode control
    void setFogRectangleModeEnabled(bool enabled);
    bool isFogRectangleModeEnabled() const { return m_fogRectangleModeEnabled; }

    // Accessors
    QGraphicsScene* getScene() const { return m_scene; }
    qreal getZoomLevel() const { return m_zoomFactor; }
    int getGridSize() const;
    GridOverlay* getGridOverlay() const { return m_gridOverlay; }
    
    // Zoom control
    void setZoomControlsEnabled(bool enabled) { m_zoomControlsEnabled = enabled; }
    void syncZoomLevel(qreal zoomLevel, const QPointF& centerPoint = QPointF());
    void setZoomLevel(qreal zoomLevel);
    void zoomToPreset(qreal zoomLevel);
    void fitMapToView();
    
    // Fog of War state management
    QByteArray saveFogState() const;
    bool loadFogState(const QByteArray& data);
    FogOfWar* getFogOverlay() const { return m_fogOverlay; }
    void connectFogChanges(QObject* receiver, const char* slot);



    // Ping tool
    void createPing(const QPointF& scenePos);

    // GM Beacon system
    void createGMBeacon(const QPointF& scenePos);

    // Lighting system
    void setLightingEnabled(bool enabled);
    void setTimeOfDay(int timeOfDay); // 0=Dawn, 1=Day, 2=Dusk, 3=Night
    void setLightingIntensity(qreal intensity);
    void setCustomLightingTint(const QColor& tint);
    void applyVTTLighting(bool globalLight, qreal darkness);
    bool isLightingEnabled() const;
    int getTimeOfDay() const;

    // Point light management
    void setPointLightPlacementMode(bool enabled);
    bool isPointLightPlacementMode() const { return m_pointLightPlacementMode; }
    void addPointLight(const QPointF& position);
    void removePointLight(const QUuid& lightId);
    void clearAllPointLights();
    void setAmbientLightLevel(qreal level);
    qreal getAmbientLightLevel() const;

    // Get lighting overlay for property editing
    LightingOverlay* getLightingOverlay();

    // Wall system for line of sight
    void setWallsEnabled(bool enabled);
    bool areWallsEnabled() const { return m_wallsEnabled; }
    void setWallDebugRenderingEnabled(bool enabled);
    bool isWallDebugRenderingEnabled() const;
    WallSystem* getWallSystem() const { return m_wallSystem; }

    // Portal system for doors/windows
    void setPortalsEnabled(bool enabled);
    bool arePortalsEnabled() const { return m_portalsEnabled; }
    void togglePortalAt(const QPointF& scenePos);
    PortalSystem* getPortalSystem() const { return m_portalSystem; }

    // Unified fog tool mode system
    void setMainWindow(MainWindow* mainWindow) { m_mainWindow = mainWindow; }
    MainWindow* getMainWindow() const { return m_mainWindow; }
    FogToolMode getCurrentFogToolMode() const;
    ToolType getCurrentTool() const { return m_currentTool; }

    // Cursor management for tools
    void setActiveTool(ToolType tool, bool isDragging = false);
    void updateCursor(ToolType tool, bool isDragging = false);
    void updateFogBrushCursor(int brushSize, FogToolMode mode);
    
public slots:
    void onToolChanged(ToolType tool);
    void onFogToolModeChanged(FogToolMode mode);
    void onFogBrushSizeChanged(int size);
    
    // Legacy cursor methods for compatibility
    void updateToolCursor();
    void updateFogBrushCursor();

    // Fog brush preview management
    void updateFogBrushPreview(const QPointF& scenePos, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    void showFogBrushPreview(bool show);

    // OpenGL rendering mode (experimental)
    void setOpenGLRenderingEnabled(bool enabled);
    bool isOpenGLRenderingEnabled() const { return m_openglRenderingEnabled; }
    OpenGLMapDisplay* getOpenGLDisplay() const { return m_openglDisplay; }

    // CRITICAL FIX: Force OpenGL refresh to recover from black screen
    void forceOpenGLRefresh();

    // Weather effects (delegates to OpenGL display)
    void setWeatherType(int weatherType);
    int getWeatherType() const;
    void setWeatherIntensity(float intensity);
    float getWeatherIntensity() const;
    void setWindDirection(float x, float y);
    void setWindStrength(float strength);

    // Post-processing effects (delegates to OpenGL display)
    void setBloomEnabled(bool enabled);
    void setBloomThreshold(float threshold);
    void setBloomIntensity(float intensity);
    void setBloomRadius(float radius);
    void setShadowMappingEnabled(bool enabled);
    void setShadowMapSize(int size);
    void setVolumetricLightingEnabled(bool enabled);
    void setVolumetricIntensity(float intensity);
    void setLightShaftsEnabled(bool enabled);
    void setLightShaftsIntensity(float intensity);
    void setMSAAEnabled(bool enabled);
    void setMSAASamples(int samples);

    // Post-processing getters (delegates to OpenGL display)
    bool isBloomEnabled() const;
    bool isShadowMappingEnabled() const;
    bool isVolumetricLightingEnabled() const;
    bool isLightShaftsEnabled() const;
    bool isMSAAEnabled() const;
    float getBloomThreshold() const;
    float getBloomIntensity() const;
    float getBloomRadius() const;
    int getShadowMapSize() const;
    float getVolumetricIntensity() const;
    float getLightShaftsIntensity() const;
    int getMSAASamples() const;

signals:
    void zoomChanged(qreal zoomLevel);
    void fogChanged();
    void scenePopulated(); // SYNC FIX: Emitted when map is loaded and scene has content
    void pointLightDoubleClicked(const QUuid& lightId);
    void toolSwitchRequested(ToolType tool);

    // CRITICAL FIX: Signal when scene is invalidated/cleared
    void sceneInvalidated();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateGrid();
    void updateFog();
    void setInitialZoom();
    void calculateReleaseVelocity();

    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_mapItem;
    GridOverlay* m_gridOverlay;
    FogOfWar* m_fogOverlay;
    WallSystem* m_wallSystem;
    PortalSystem* m_portalSystem;

    QImage m_currentMap;
    bool m_gridEnabled;
    bool m_fogEnabled;
    bool m_wallsEnabled;
    bool m_portalsEnabled;
    bool m_ownScene;  // Whether this display owns its scene
    int m_vttGridSize;  // Grid size from VTT data (0 for non-VTT files)
    int m_fogBrushSize;  // Current fog brush size in pixels
    bool m_fogHideModeEnabled;  // Whether fog hide mode is active
    bool m_fogRectangleModeEnabled;  // Whether fog rectangle mode is active

    // Rectangle selection state
    bool m_isSelectingRectangle;
    QPointF m_rectangleStartPos;
    QRectF m_currentSelectionRect;
    class QGraphicsRectItem* m_selectionRectIndicator;

    // Pan support with momentum
    bool m_isPanning;
    QPoint m_lastPanPoint;

    // Velocity tracking for momentum
    QList<QPointF> m_velocityHistory;
    QList<qint64> m_velocityTimestamps;
    qint64 m_lastMoveTime;

    // Zoom support
    qreal m_zoomFactor;
    qreal m_targetZoomFactor;
    static constexpr qreal MIN_ZOOM = 0.1;
    static constexpr qreal MAX_ZOOM = 5.0;

    // Animation support
    class QPropertyAnimation* m_zoomAnimation;
    class QTimer* m_smoothPanTimer;
    QPointF m_panVelocity;
    QPointF m_lastMousePos;
    
    // Smooth zoom animation state
    qreal m_animationStartZoom;
    qreal m_animationTargetZoom;
    QPointF m_zoomAnimationCenter;
    bool m_isZoomAnimating;
    class QTimer* m_zoomAccumulationTimer;

    // Zoom indicator
    ZoomIndicator* m_zoomIndicator;

    // Loading progress indicator
    LoadingProgressWidget* m_loadingProgressWidget;
    ImageLoader* m_imageLoader;

    // Zoom cursor tracking (to avoid lambda capture issues)
    QPointF m_zoomCursorPos;
    QPointF m_zoomScenePos;
    bool m_zoomCenterOnCursor; // Controls centering behavior during zoom animation
    
    // Zoom control state
    bool m_zoomControlsEnabled;


    // Lighting effects
    LightingOverlay* m_lightingOverlay;

    // Point light placement mode
    bool m_pointLightPlacementMode;
    QUuid m_selectedPointLightId;
    class QGraphicsEllipseItem* m_selectedPointLightIndicator;

    // Reference to main window for tool mode queries
    MainWindow* m_mainWindow;

    // Fog brush preview circle
    class QGraphicsEllipseItem* m_fogBrushPreview;

    // Brush size HUD overlay
    class QLabel* m_brushSizeHUD;
    class QTimer* m_hudFadeTimer;
    void showBrushSizeHUD(int size);
    void hideBrushSizeHUD();

    // Current tool state for cursor management
    ToolType m_currentTool;
    FogToolMode m_currentFogMode;
    bool m_isDraggingTool;

    // OpenGL rendering support (experimental)
    // Note: Uses Qt parent-child ownership - no manual deletion needed
    bool m_openglRenderingEnabled;
    OpenGLMapDisplay* m_openglDisplay;

private slots:
    void animateZoom();
    void updateSmoothPan();
    void finishZoomAccumulation();

public:
    // Debug rendering of parsed VTT lights
    void setParsedLights(const QList<VTTLoader::LightSource>& lights);
    void setShowParsedLights(bool enabled);

private:
    QList<VTTLoader::LightSource> m_parsedLights;
    QList<class QGraphicsEllipseItem*> m_lightDebugItems;
    bool m_showParsedLights = false;
    void updateParsedLightOverlays();

    // Thread safety for shared scene access
    static QRecursiveMutex s_sceneMutex;  // Shared recursive mutex for all MapDisplay instances

    // Static flag to track if app is ready for progress events
    static bool s_appReadyForProgress;
};

#endif // MAPDISPLAY_H
