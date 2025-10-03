#include "graphics/MapDisplay.h"
#include "opengl/OpenGLMapDisplay.h"
#include "ui/MainWindow.h"
#include "utils/AnimationHelper.h"
#include <QMessageBox>
#include <QApplication>
#include <iostream>

// OpenGL is mandatory
extern const bool g_useOpenGL;
#include "graphics/GridOverlay.h"
#include "graphics/FogOfWar.h"
#include "graphics/WallSystem.h"
#include "graphics/PortalSystem.h"
#include "graphics/PingIndicator.h"
#include "graphics/GMBeacon.h"
#include "graphics/LightingOverlay.h"
#include "graphics/ZoomIndicator.h"
#include "graphics/LoadingProgressWidget.h"
#include "utils/ImageLoader.h"
#include "utils/VTTLoader.h"
#include "utils/DebugConsole.h"
#include "utils/CustomCursors.h"
#include "utils/FogToolMode.h"
#include "utils/ToolType.h"
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QKeyEvent>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPainter>
#include <QGraphicsTextItem>
#include <QDateTime>
#include <QApplication>
#include <QThread>
#include <QEventLoop>
#include <cmath>
#include <QtMath>

// Define static recursive mutex for thread-safe scene access
QRecursiveMutex MapDisplay::s_sceneMutex;

// Define static flag for app readiness
bool MapDisplay::s_appReadyForProgress = false;

MapDisplay::MapDisplay(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_mapItem(nullptr)
    , m_gridOverlay(nullptr)
    , m_fogOverlay(nullptr)
    , m_wallSystem(nullptr)
    , m_portalSystem(nullptr)
    , m_gridEnabled(true)
    , m_fogEnabled(true)
    , m_wallsEnabled(false)
    , m_portalsEnabled(false)
    , m_ownScene(true)
    , m_vttGridSize(0)
    , m_fogBrushSize(200)  // Default brush size (50% of max 400)
    , m_fogHideModeEnabled(false)
    , m_fogRectangleModeEnabled(false)
    , m_isSelectingRectangle(false)
    , m_selectionRectIndicator(nullptr)
    , m_isPanning(false)
    , m_lastMoveTime(0)
    , m_zoomFactor(1.0)
    , m_targetZoomFactor(1.0)
    , m_zoomAnimation(nullptr)
    , m_smoothPanTimer(nullptr)
    , m_animationStartZoom(1.0)
    , m_animationTargetZoom(1.0)
    , m_isZoomAnimating(false)
    , m_zoomAccumulationTimer(nullptr)
    , m_zoomIndicator(nullptr)
    , m_zoomControlsEnabled(true)
    , m_lightingOverlay(nullptr)
    , m_mainWindow(nullptr)
    , m_pointLightPlacementMode(false)
    , m_selectedPointLightIndicator(nullptr)
    , m_loadingProgressWidget(nullptr)
    , m_imageLoader(nullptr)
    , m_fogBrushPreview(nullptr)
    , m_brushSizeHUD(nullptr)
    , m_hudFadeTimer(nullptr)
    , m_currentTool(ToolType::Pointer)
    , m_currentFogMode(FogToolMode::UnifiedFog)
    , m_isDraggingTool(false)
    , m_openglRenderingEnabled(false)
    , m_openglDisplay(nullptr)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    setDragMode(QGraphicsView::NoDrag);

    // CRITICAL FIX: Use black background for letterboxing support
    // When map doesn't match screen aspect ratio, black bars look professional
    setBackgroundBrush(QBrush(QColor(0, 0, 0)));

    setFocusPolicy(Qt::ClickFocus);

    m_gridOverlay = new GridOverlay();
    m_fogOverlay = new FogOfWar();
    m_fogOverlay->setChangeCallback([this](const QRectF& dirtyRegion) {
        notifyFogChanged(dirtyRegion);
    });

    // Initialize lighting effects with lazy loading
    m_lightingOverlay = nullptr;

    m_smoothPanTimer = new QTimer(this);
    m_smoothPanTimer->setInterval(16);
    connect(m_smoothPanTimer, &QTimer::timeout, this, &MapDisplay::updateSmoothPan);
    
    // Initialize zoom accumulation timer for smooth zoom handling
    m_zoomAccumulationTimer = new QTimer(this);
    m_zoomAccumulationTimer->setSingleShot(true);
    m_zoomAccumulationTimer->setInterval(50); // 50ms delay for zoom accumulation
    connect(m_zoomAccumulationTimer, &QTimer::timeout, this, &MapDisplay::finishZoomAccumulation);

    m_zoomAnimation = new QPropertyAnimation(this);
    m_zoomAnimation->setDuration(250);
    m_zoomAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_zoomAnimation, &QPropertyAnimation::valueChanged, this, &MapDisplay::animateZoom);

    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);
    
    // Create zoom indicator
    m_zoomIndicator = new ZoomIndicator(this);

    // Create loading progress widget
    m_loadingProgressWidget = new LoadingProgressWidget(this);

    // Create image loader
    m_imageLoader = new ImageLoader(this);
    connect(m_imageLoader, &ImageLoader::progressChanged,
            m_loadingProgressWidget, &LoadingProgressWidget::setProgress);
    connect(m_imageLoader, &ImageLoader::statusChanged,
            m_loadingProgressWidget, &LoadingProgressWidget::setLoadingText);

    // Create fog brush preview circle
    m_fogBrushPreview = new QGraphicsEllipseItem();
    m_fogBrushPreview->setVisible(false);
    m_fogBrushPreview->setZValue(1000); // High Z-value to show above everything

    // THREAD SAFETY: Protect scene modification
    {
        QMutexLocker locker(&s_sceneMutex);
        m_scene->addItem(m_fogBrushPreview);
    }

    // Create brush size HUD overlay
    m_brushSizeHUD = new QLabel(this);
    m_brushSizeHUD->setStyleSheet(
        "QLabel {"
        "   background-color: rgba(0, 0, 0, 180);"
        "   color: white;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   padding: 12px 20px;"
        "   border-radius: 8px;"
        "   border: 2px solid rgba(255, 255, 255, 0.3);"
        "}"
    );
    m_brushSizeHUD->setAlignment(Qt::AlignCenter);
    m_brushSizeHUD->setVisible(false);

    // Create fade timer for HUD
    m_hudFadeTimer = new QTimer(this);
    m_hudFadeTimer->setSingleShot(true);
    m_hudFadeTimer->setInterval(2000); // 2 seconds
    connect(m_hudFadeTimer, &QTimer::timeout, this, &MapDisplay::hideBrushSizeHUD);

    std::cerr << "[MapDisplay::MapDisplay] OpenGL disabled for stability" << std::endl;
}

MapDisplay::~MapDisplay()
{
    // Stop all timers and animations
    if (m_zoomAnimation) {
        m_zoomAnimation->stop();
    }
    if (m_smoothPanTimer) {
        m_smoothPanTimer->stop();
    }
    if (m_zoomAccumulationTimer) {
        m_zoomAccumulationTimer->stop();
    }
    if (m_hudFadeTimer) {
        m_hudFadeTimer->stop();
    }

    // Reset animation state
    m_isZoomAnimating = false;

    // Only manually delete items if we own the scene AND they're not in the scene
    // Scene items are automatically deleted when scene is deleted (Qt parent-child)
    if (m_ownScene && m_scene) {
        // Items in scene will be deleted automatically by Qt
        // Only need to clear our pointers
        m_gridOverlay = nullptr;
        m_fogOverlay = nullptr;
        m_wallSystem = nullptr;
        m_portalSystem = nullptr;  // CRITICAL FIX: Ensure consistency
        m_lightingOverlay = nullptr;
        m_mapItem = nullptr;
        m_fogBrushPreview = nullptr;
        m_selectionRectIndicator = nullptr;
        m_selectedPointLightIndicator = nullptr;
        m_lightDebugItems.clear();
    }

    // OpenGL display will be cleaned up automatically by Qt's parent-child system
}

bool MapDisplay::loadImage(const QString& path)
{
    DebugConsole::info(QString("Loading image: %1").arg(path), "Loading");
    DebugConsole::info(QString("Loading thread: %1").arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16), "Loading");

    // Check if it's a VTT file
    if (VTTLoader::isVTTFile(path)) {
        DebugConsole::vtt("Detected VTT file, calling VTTLoader::loadVTT() synchronously", "VTT Parsing");
        VTTLoader::VTTData vttData = VTTLoader::loadVTT(path, nullptr);

        // Check if we got a valid image, even if other VTT data failed
        if (!vttData.mapImage.isNull()) {
            m_currentMap = vttData.mapImage;

            // Store VTT grid information if available
            if (vttData.isValid) {
                m_vttGridSize = vttData.pixelsPerGrid;
            } else {
                m_vttGridSize = 0;  // Use default grid if VTT data invalid
                DebugConsole::warning(QString("VTT file loaded with warnings: %1").arg(vttData.errorMessage), "Graphics");
            }
        } else {
            DebugConsole::error(QString("Failed to load VTT file: %1").arg(vttData.errorMessage), "Graphics");
            return false;
        }
    } else {
        // Load regular image
        m_currentMap.load(path);
        m_vttGridSize = 0;  // Reset VTT grid size for non-VTT files
    }

    if (m_currentMap.isNull()) {
        return false;
    }

    // Save fog state before clearing
    QByteArray fogState;
    if (m_fogOverlay) {
        fogState = m_fogOverlay->saveState();
    }

    if (m_lightingOverlay) {
        m_lightingOverlay->setEnabled(false);
        // Point light feature removed - no need to clear lights
    }

    // Clear scene - this deletes all items
    // CRITICAL: Lock mutex for thread-safe scene modification
    QMutexLocker locker(&s_sceneMutex);
    m_scene->clear();

    // CRITICAL: Reset ALL pointers since scene->clear() deleted them
    // This prevents double-delete and use-after-free bugs
    m_gridOverlay = nullptr;
    m_fogOverlay = nullptr;
    m_wallSystem = nullptr;
    m_portalSystem = nullptr;  // CRITICAL FIX: Was missing, causing memory leak
    m_mapItem = nullptr;
    m_fogBrushPreview = nullptr;
    m_lightingOverlay = nullptr;  // FIX: Was missing, causing memory leak
    m_selectionRectIndicator = nullptr;
    m_selectedPointLightIndicator = nullptr;


    // Clear light debug items
    m_lightDebugItems.clear();

    // Create new map item
    m_loadingProgressWidget->setProgress(55);
    m_loadingProgressWidget->setLoadingText("Creating map display...");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);

    QPixmap pixmap = QPixmap::fromImage(m_currentMap);

    m_loadingProgressWidget->setProgress(60);
    m_loadingProgressWidget->setLoadingText("Adding map to scene...");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);

    // THREAD SAFETY: Protect scene modification
    {
        QMutexLocker locker(&s_sceneMutex);
        m_mapItem = m_scene->addPixmap(pixmap);
        m_mapItem->setZValue(0);
        m_scene->setSceneRect(pixmap.rect());
    }

    // CRITICAL DIAGNOSTIC: Verify image rendering pipeline
    // These checks catch the most common rendering failures immediately
    if (!m_mapItem) {
        DebugConsole::error("CRITICAL: Map item is nullptr after creation!", "Rendering");
        return false;
    }

    if (!m_scene->items().contains(m_mapItem)) {
        DebugConsole::error("CRITICAL: Map item not in scene after adding!", "Rendering");
        return false;
    }

    if (m_mapItem->zValue() != 0) {
        DebugConsole::warning(QString("Map item z-value incorrect: %1 (should be 0)").arg(m_mapItem->zValue()), "Rendering");
    }

    if (!m_mapItem->isVisible()) {
        DebugConsole::error("CRITICAL: Map item is not visible!", "Rendering");
        m_mapItem->setVisible(true);
    }

    if (m_scene->sceneRect() != pixmap.rect()) {
        DebugConsole::warning("Scene rect doesn't match image rect", "Rendering");
    }

    // Log successful rendering for test verification
    DebugConsole::info("IMAGE_RENDERED_SUCCESS: Map item added to scene", "Rendering");
    DebugConsole::info(QString("Image dimensions: %1x%2").arg(pixmap.width()).arg(pixmap.height()), "Rendering");
    DebugConsole::info(QString("Scene rect: %1x%2").arg(m_scene->width()).arg(m_scene->height()), "Rendering");

    m_loadingProgressWidget->setProgress(65);
    m_loadingProgressWidget->setLoadingText("Setting up overlays...");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);

    // Recreate grid overlay
    m_gridOverlay = new GridOverlay();
    m_gridOverlay->setMapSize(pixmap.size());

    // Apply VTT grid size if available
    if (m_vttGridSize > 0) {
        m_gridOverlay->setGridSize(m_vttGridSize);
    }

    if (m_gridEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_gridOverlay);
            m_gridOverlay->setZValue(1);
        }
    }

    // Recreate wall system first (needed by fog overlay)
    m_wallSystem = new WallSystem();
    m_wallSystem->setMapSize(pixmap.size());
    if (m_vttGridSize > 0) {
        m_wallSystem->setPixelsPerGrid(m_vttGridSize);
    }
    if (m_wallsEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_wallSystem);
            m_wallSystem->setZValue(5);
        }
    }

    // Recreate portal system
    m_portalSystem = new PortalSystem();
    m_portalSystem->setMapSize(pixmap.size());
    if (m_vttGridSize > 0) {
        m_portalSystem->setPixelsPerGrid(m_vttGridSize);
    }
    if (m_portalsEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_portalSystem);
            m_portalSystem->setZValue(10);
        }
    }

    // Connect portal system to wall system for line-of-sight integration
    m_wallSystem->setPortalSystem(m_portalSystem);

    // Recreate fog overlay and connect to wall system
    m_fogOverlay = new FogOfWar();
    m_fogOverlay->setMapSize(pixmap.size());
    m_fogOverlay->setWallSystem(m_wallSystem);  // Connect wall system for line-of-sight
    if (!fogState.isEmpty()) {
        m_fogOverlay->loadState(fogState);
    }
    // Reconnect fog change callback
    m_fogOverlay->setChangeCallback([this](const QRectF& dirtyRegion) {
        notifyFogChanged(dirtyRegion);
    });

    // Recreate drawing overlay
    if (m_fogEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_fogOverlay);
            m_fogOverlay->setZValue(2);
        }
    }

    // Recreate fog brush preview circle
    m_fogBrushPreview = new QGraphicsEllipseItem();
    m_fogBrushPreview->setVisible(false);
    m_fogBrushPreview->setZValue(1000); // High Z-value to show above everything
    // THREAD SAFETY: Protect scene modification
    {
        QMutexLocker locker(&s_sceneMutex);
        m_scene->addItem(m_fogBrushPreview);
    }

    // Recreate lighting overlay (was deleted by scene->clear())
    // NOTE: Using lazy loading pattern to avoid unnecessary creation
    m_lightingOverlay = nullptr;  // Will be created on demand via getLightingOverlay()

    // Delay initial zoom to ensure view is properly sized
    QTimer::singleShot(10, this, [this]() {
        setInitialZoom();
    });

    // Ensure overlays are properly added to scene
    updateGrid();
    updateFog();

    // Ensure all connected views get the initial fog state immediately
    notifyFogChanged();

    // SYNC FIX: Notify that scene now has content
    emit scenePopulated();

    return true;
}

bool MapDisplay::loadImageWithProgress(const QString& path)
{
    // Show progress widget
    m_loadingProgressWidget->showProgress();
    m_loadingProgressWidget->setProgress(0);
    m_loadingProgressWidget->setLoadingText("Preparing to load...");

    // CRITICAL FIX: Avoid multiple VTT loading
    // Load VTT data once and reuse it throughout the method
    VTTLoader::VTTData vttData;
    bool isVTTFile = VTTLoader::isVTTFile(path);

    if (isVTTFile) {
        m_loadingProgressWidget->setLoadingText("Loading VTT file...");
        m_loadingProgressWidget->setProgress(25);

        // Pass progress callback to VTTLoader
        // CRITICAL: Only process events if app is ready (after first paint)
        auto progressCallback = [this](int percentage, const QString& message) {
            m_loadingProgressWidget->setProgress(percentage);
            m_loadingProgressWidget->setLoadingText(message);

            // Only call processEvents if the app has been painted at least once
            // This prevents hangs when DD2VTT is loaded as the first file
            if (s_appReadyForProgress) {
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        };

        vttData = VTTLoader::loadVTT(path, progressCallback);
        if (vttData.mapImage.isNull()) {
            m_loadingProgressWidget->hideProgress();
            DebugConsole::error(QString("Failed to load VTT file: %1").arg(vttData.errorMessage), "Graphics");
            return false;
        }

        m_currentMap = vttData.mapImage;
        m_vttGridSize = vttData.isValid ? vttData.pixelsPerGrid : 0;

        m_loadingProgressWidget->setProgress(50);
        m_loadingProgressWidget->setLoadingText("Processing VTT data...");
    } else {
        // Use image loader for regular images
        m_loadingProgressWidget->setProgress(25);
        QImage loadedImage = m_imageLoader->loadImageWithProgress(path);

        if (loadedImage.isNull()) {
            m_loadingProgressWidget->hideProgress();
            return false;
        }

        m_currentMap = loadedImage;
        m_vttGridSize = 0;
        m_loadingProgressWidget->setProgress(50);
    }

    // Save fog state before clearing
    QByteArray fogState;
    if (m_fogOverlay) {
        fogState = m_fogOverlay->saveState();
    }

    if (m_lightingOverlay) {
        m_lightingOverlay->setEnabled(false);
        // Point light feature removed - no need to clear lights
    }

    // Clear scene - this deletes all items
    // CRITICAL: Lock mutex for thread-safe scene modification
    QMutexLocker locker(&s_sceneMutex);
    m_scene->clear();

    // CRITICAL: Reset ALL pointers since scene->clear() deleted them
    // This prevents double-delete and use-after-free bugs
    m_gridOverlay = nullptr;
    m_fogOverlay = nullptr;
    m_wallSystem = nullptr;
    m_portalSystem = nullptr;  // CRITICAL FIX: Was missing, causing memory leak
    m_mapItem = nullptr;
    m_fogBrushPreview = nullptr;
    m_lightingOverlay = nullptr;  // FIX: Was missing, causing memory leak
    m_selectionRectIndicator = nullptr;
    m_selectedPointLightIndicator = nullptr;


    // Clear light debug items
    m_lightDebugItems.clear();

    // Create new map item
    m_loadingProgressWidget->setProgress(55);
    m_loadingProgressWidget->setLoadingText("Creating map display...");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);

    QPixmap pixmap = QPixmap::fromImage(m_currentMap);

    m_loadingProgressWidget->setProgress(60);
    m_loadingProgressWidget->setLoadingText("Adding map to scene...");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);

    // THREAD SAFETY: Protect scene modification
    {
        QMutexLocker locker(&s_sceneMutex);
        m_mapItem = m_scene->addPixmap(pixmap);
        m_mapItem->setZValue(0);
        m_scene->setSceneRect(pixmap.rect());
    }

    // CRITICAL DIAGNOSTIC: Verify image rendering pipeline
    // These checks catch the most common rendering failures immediately
    if (!m_mapItem) {
        DebugConsole::error("CRITICAL: Map item is nullptr after creation!", "Rendering");
        return false;
    }

    if (!m_scene->items().contains(m_mapItem)) {
        DebugConsole::error("CRITICAL: Map item not in scene after adding!", "Rendering");
        return false;
    }

    if (m_mapItem->zValue() != 0) {
        DebugConsole::warning(QString("Map item z-value incorrect: %1 (should be 0)").arg(m_mapItem->zValue()), "Rendering");
    }

    if (!m_mapItem->isVisible()) {
        DebugConsole::error("CRITICAL: Map item is not visible!", "Rendering");
        m_mapItem->setVisible(true);
    }

    if (m_scene->sceneRect() != pixmap.rect()) {
        DebugConsole::warning("Scene rect doesn't match image rect", "Rendering");
    }

    // Log successful rendering for test verification
    DebugConsole::info("IMAGE_RENDERED_SUCCESS: Map item added to scene", "Rendering");
    DebugConsole::info(QString("Image dimensions: %1x%2").arg(pixmap.width()).arg(pixmap.height()), "Rendering");
    DebugConsole::info(QString("Scene rect: %1x%2").arg(m_scene->width()).arg(m_scene->height()), "Rendering");

    m_loadingProgressWidget->setProgress(65);
    m_loadingProgressWidget->setLoadingText("Setting up overlays...");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);

    // Recreate grid overlay
    m_gridOverlay = new GridOverlay();
    m_gridOverlay->setMapSize(pixmap.size());

    // Apply VTT grid size if available
    if (m_vttGridSize > 0) {
        m_gridOverlay->setGridSize(m_vttGridSize);
    }

    if (m_gridEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_gridOverlay);
            m_gridOverlay->setZValue(1);
        }
    }

    // Recreate wall system first (needed by fog overlay)
    m_wallSystem = new WallSystem();
    m_wallSystem->setMapSize(pixmap.size());
    if (m_vttGridSize > 0) {
        m_wallSystem->setPixelsPerGrid(m_vttGridSize);
    }
    if (m_wallsEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_wallSystem);
            m_wallSystem->setZValue(5);
        }
    }

    // Recreate portal system
    m_portalSystem = new PortalSystem();
    m_portalSystem->setMapSize(pixmap.size());
    if (m_vttGridSize > 0) {
        m_portalSystem->setPixelsPerGrid(m_vttGridSize);
    }
    if (m_portalsEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_portalSystem);
            m_portalSystem->setZValue(10);
        }
    }

    // Connect portal system to wall system for line-of-sight integration
    m_wallSystem->setPortalSystem(m_portalSystem);

    // Recreate fog overlay and connect to wall system
    m_fogOverlay = new FogOfWar();
    m_fogOverlay->setMapSize(pixmap.size());
    m_fogOverlay->setWallSystem(m_wallSystem);  // Connect wall system for line-of-sight
    if (!fogState.isEmpty()) {
        m_fogOverlay->loadState(fogState);
    }
    // Reconnect fog change callback
    m_fogOverlay->setChangeCallback([this](const QRectF& dirtyRegion) {
        notifyFogChanged(dirtyRegion);
    });

    // Recreate drawing overlay
    if (m_fogEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_fogOverlay);
            m_fogOverlay->setZValue(2);
        }
    }

    // Recreate fog brush preview circle
    m_fogBrushPreview = new QGraphicsEllipseItem();
    m_fogBrushPreview->setVisible(false);
    m_fogBrushPreview->setZValue(1000); // High Z-value to show above everything
    // THREAD SAFETY: Protect scene modification
    {
        QMutexLocker locker(&s_sceneMutex);
        m_scene->addItem(m_fogBrushPreview);
    }

    // Use lazy loading for lighting effects
    // m_lightingOverlay will be created on demand via getLightingOverlay()
    m_lightingOverlay = nullptr;

    // Apply VTT lighting and parsed lights if available
    // CRITICAL FIX: Use already loaded VTT data instead of loading again
    if (isVTTFile && vttData.isValid) {
        m_loadingProgressWidget->setProgress(75);
        m_loadingProgressWidget->setLoadingText("Applying VTT features...");

        applyVTTLighting(vttData.globalLight, vttData.darkness);
        setParsedLights(vttData.lights);

        // Load wall data from VTT file
        if (!vttData.walls.isEmpty() && m_wallSystem) {
            QList<WallSystem::Wall> walls;
            for (const auto& vttWall : vttData.walls) {
                walls.append(WallSystem::Wall(vttWall.line));
            }
            m_wallSystem->setWalls(walls);
            DebugConsole::vtt(QString("Loaded %1 walls from VTT file").arg(walls.size()), "VTT Parsing");
        }

        // Load portal data from VTT file
        if (!vttData.portals.isEmpty() && m_portalSystem) {
            QList<PortalSystem::PortalData> portals;
            for (const auto& vttPortal : vttData.portals) {
                PortalSystem::PortalData portal(vttPortal.position, vttPortal.bound1, vttPortal.bound2,
                                               vttPortal.rotation, vttPortal.closed, vttPortal.freestanding);
                portals.append(portal);
            }
            m_portalSystem->setPortals(portals);
            DebugConsole::vtt(QString("Loaded %1 portals from VTT file").arg(portals.size()), "Graphics");
        }
    }

    // CRITICAL FIX: Ensure progress reaches 100% before hiding
    m_loadingProgressWidget->setProgress(100);
    m_loadingProgressWidget->setLoadingText("Loading complete");

    // Hide progress widget after a brief moment to show completion
    QTimer::singleShot(200, this, [this]() {
        m_loadingProgressWidget->hideProgress();
    });

    // Delay initial zoom to ensure view is properly sized
    QTimer::singleShot(250, this, [this]() {
        setInitialZoom();
    });

    // CRITICAL FIX: Ensure image is visible regardless of OpenGL state
    // Force update of the scene to ensure map item is properly rendered
    if (m_mapItem) {
        m_mapItem->setVisible(true);
        m_mapItem->update();
    }
    
    // Force scene update and view refresh
    if (m_scene) {
        m_scene->update();
    }
    update();
    

    // Ensure overlays are properly added to scene
    updateGrid();
    updateFog();

    // Ensure all connected views get the initial fog state immediately
    notifyFogChanged();

    // SYNC FIX: Notify that scene now has content
    emit scenePopulated();

    return true;
}

bool MapDisplay::loadImageFromCache(const QImage& cachedImage, const VTTLoader::VTTData& vttData)
{
    if (cachedImage.isNull()) {
        DebugConsole::warning("loadImageFromCache() - Cached image is null", "Graphics");
        return false;
    }

    DebugConsole::performance("Loading from cached image (fast path)", "Loading");

    // Show progress briefly to provide user feedback
    m_loadingProgressWidget->showProgress();
    m_loadingProgressWidget->setProgress(25);
    m_loadingProgressWidget->setLoadingText("Loading from cache...");

    // Use cached image
    m_currentMap = cachedImage;

    // Set VTT grid size from cached data
    m_vttGridSize = vttData.isValid ? vttData.pixelsPerGrid : 0;

    // Save fog state before clearing
    QByteArray fogState;
    if (m_fogOverlay) {
        fogState = m_fogOverlay->saveState();
    }

    if (m_lightingOverlay) {
        m_lightingOverlay->setEnabled(false);
        // Point light feature removed - no lights to clear
    }

    m_loadingProgressWidget->setProgress(50);

    // Clear scene - this deletes all items
    QMutexLocker locker(&s_sceneMutex);
    m_scene->clear();

    // Reset ALL pointers since scene->clear() deleted them
    m_gridOverlay = nullptr;
    m_fogOverlay = nullptr;
    m_wallSystem = nullptr;
    m_portalSystem = nullptr;
    m_mapItem = nullptr;
    m_fogBrushPreview = nullptr;
    m_lightingOverlay = nullptr;
    m_selectionRectIndicator = nullptr;
    m_selectedPointLightIndicator = nullptr;
    m_lightDebugItems.clear();

    m_loadingProgressWidget->setProgress(75);

    // Create new map item from cached image
    // THREAD SAFETY: Protect scene modification
    {
        QMutexLocker locker(&s_sceneMutex);
        m_mapItem = m_scene->addPixmap(QPixmap::fromImage(m_currentMap));
        if (!m_mapItem) {
            m_loadingProgressWidget->hideProgress();
            return false;
        }
        // Set scene rect
        m_scene->setSceneRect(m_currentMap.rect());
    }

    // Initialize all overlays (same as original implementation)
    QPixmap pixmap = QPixmap::fromImage(m_currentMap);

    // Recreate grid overlay
    m_gridOverlay = new GridOverlay();
    m_gridOverlay->setMapSize(pixmap.size());

    // Apply VTT grid size if available
    if (m_vttGridSize > 0) {
        m_gridOverlay->setGridSize(m_vttGridSize);
    }

    if (m_gridEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_gridOverlay);
            m_gridOverlay->setZValue(1);
        }
    }

    // Recreate wall system first (needed by fog overlay)
    m_wallSystem = new WallSystem();
    m_wallSystem->setMapSize(pixmap.size());
    if (m_vttGridSize > 0) {
        m_wallSystem->setPixelsPerGrid(m_vttGridSize);
    }
    if (m_wallsEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_wallSystem);
            m_wallSystem->setZValue(5);
        }
    }

    // Recreate portal system
    m_portalSystem = new PortalSystem();
    m_portalSystem->setMapSize(pixmap.size());
    if (m_vttGridSize > 0) {
        m_portalSystem->setPixelsPerGrid(m_vttGridSize);
    }
    if (m_portalsEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_portalSystem);
            m_portalSystem->setZValue(10);
        }
    }

    // Connect portal system to wall system for line-of-sight integration
    m_wallSystem->setPortalSystem(m_portalSystem);

    // Recreate fog overlay and connect to wall system
    m_fogOverlay = new FogOfWar();
    m_fogOverlay->setMapSize(pixmap.size());
    m_fogOverlay->setWallSystem(m_wallSystem);  // Connect wall system for line-of-sight

    // Reconnect fog change callback
    m_fogOverlay->setChangeCallback([this](const QRectF& dirtyRegion) {
        notifyFogChanged(dirtyRegion);
    });

    // Recreate drawing overlay

    if (m_fogEnabled) {
        // THREAD SAFETY: Protect scene modification
        {
            QMutexLocker locker(&s_sceneMutex);
            m_scene->addItem(m_fogOverlay);
            m_fogOverlay->setZValue(2);
        }
    }

    // Recreate fog brush preview circle
    m_fogBrushPreview = new QGraphicsEllipseItem();
    m_fogBrushPreview->setVisible(false);
    m_fogBrushPreview->setZValue(1000); // High Z-value to show above everything
    // THREAD SAFETY: Protect scene modification
    {
        QMutexLocker locker(&s_sceneMutex);
        m_scene->addItem(m_fogBrushPreview);
    }

    // Use lazy loading for lighting effects
    m_lightingOverlay = nullptr;

    // Process VTT data if available
    if (vttData.isValid) {
        // Apply VTT lighting data
        setParsedLights(vttData.lights);

        // Apply VTT wall data
        if (!vttData.walls.isEmpty() && m_wallSystem) {
            QList<WallSystem::Wall> walls;
            for (const auto& vttWall : vttData.walls) {
                walls.append(WallSystem::Wall(vttWall.line));
            }
            m_wallSystem->setWalls(walls);
            DebugConsole::vtt(QString("Loaded %1 walls from VTT file").arg(walls.size()), "VTT Parsing");
        }

        // Apply VTT portal data
        if (!vttData.portals.isEmpty() && m_portalSystem) {
            QList<PortalSystem::PortalData> portals;
            for (const auto& vttPortal : vttData.portals) {
                PortalSystem::PortalData portal(vttPortal.position, vttPortal.bound1, vttPortal.bound2,
                                               vttPortal.rotation, vttPortal.closed, vttPortal.freestanding);
                portals.append(portal);
            }
            m_portalSystem->setPortals(portals);
            DebugConsole::vtt(QString("Loaded %1 portals from VTT file").arg(portals.size()), "Graphics");
        }
    }

    m_loadingProgressWidget->setProgress(90);

    // Update shared displays
    updateSharedScene();

    m_loadingProgressWidget->setProgress(100);
    m_loadingProgressWidget->hideProgress();

    // Set initial zoom after a brief delay for UI responsiveness
    QTimer::singleShot(50, this, [this]() {
        setInitialZoom();
    });

    // CRITICAL FIX: Ensure cached image visibility
    // Force map item visibility and scene refresh
    if (m_mapItem) {
        m_mapItem->setVisible(true);
        m_mapItem->update();
    }
    
    // Ensure scene is properly updated
    if (m_scene) {
        m_scene->update();
    }
    update();
    

    // Ensure overlays are properly added to scene
    updateGrid();
    updateFog();

    DebugConsole::performance("Successfully loaded from cache", "Graphics");
    return true;
}

void MapDisplay::setCachedImage(const QImage& image)
{
    m_currentMap = image;
}

void MapDisplay::setParsedLights(const QList<VTTLoader::LightSource>& lights)
{
    m_parsedLights = lights;
    updateParsedLightOverlays();

    // Convert VTT lights to PointLight objects and add to lighting overlay
    // Point light feature removed - VTT lights not supported
    Q_UNUSED(lights);
}

void MapDisplay::setShowParsedLights(bool enabled)
{
    m_showParsedLights = enabled;
    updateParsedLightOverlays();
}

void MapDisplay::updateParsedLightOverlays()
{
    // Clear previous debug markers
    if (!m_lightDebugItems.isEmpty()) {
        for (auto* item : m_lightDebugItems) {
            if (item && m_scene) {
                // THREAD SAFETY: Protect scene modification
                QMutexLocker locker(&s_sceneMutex);
                m_scene->removeItem(item);
            }
            delete item;
        }
        m_lightDebugItems.clear();
    }
    if (!m_showParsedLights || !m_scene || m_parsedLights.isEmpty()) return;

    // Add simple circles at light positions; scale radii for visibility
    for (const auto& l : m_parsedLights) {
        qreal r = qMax<qreal>(8.0, l.brightRadius);
        QRectF rect(l.position.x() - r, l.position.y() - r, r*2, r*2);
        // THREAD SAFETY: Protect scene modification
        QGraphicsEllipseItem* ellipse;
        {
            QMutexLocker locker(&s_sceneMutex);
            ellipse = m_scene->addEllipse(rect, QPen(QColor(255, 255, 0, 200), 2), QBrush(Qt::NoBrush));
            ellipse->setZValue(700);
        }
        m_lightDebugItems.append(ellipse);
    }
}

void MapDisplay::updateFogBrushPreview(const QPointF& scenePos, Qt::KeyboardModifiers modifiers)
{
    if (!m_fogBrushPreview || !m_fogEnabled) {
        return;
    }

    // Use provided modifiers or fall back to current keyboard state
    if (modifiers == Qt::NoModifier) {
        modifiers = QApplication::keyboardModifiers();
    }

    // CRITICAL: m_fogBrushSize is the diameter, not radius
    // The preview circle must match EXACTLY what will be painted
    qreal diameter = m_fogBrushSize;
    qreal radius = diameter / 2.0;

    // Center the circle at the scene position
    QRectF rect(scenePos.x() - radius, scenePos.y() - radius,
                diameter, diameter);
    m_fogBrushPreview->setRect(rect);

    // Set color based on modifier keys for unified fog mode (Photoshop-style)
    FogToolMode mode = getCurrentFogToolMode();
    // Always green for reveal mode (no hide mode)
    QColor previewColor = QColor(100, 255, 100, 128);

    // Create pen for the circle outline with better contrast
    QPen previewPen(previewColor, 4); // Thicker outline for better visibility
    previewPen.setStyle(Qt::SolidLine);
    m_fogBrushPreview->setPen(previewPen);

    // Add a more visible semi-transparent fill
    QColor fillColor = previewColor;
    fillColor.setAlpha(40); // More visible fill for better contrast
    m_fogBrushPreview->setBrush(QBrush(fillColor));

    // Note: For a true center dot, we'd need a separate graphics item.
    // The circle itself serves as the visual indicator.
}

void MapDisplay::showFogBrushPreview(bool show)
{
    if (m_fogBrushPreview) {
        // Determine if we should show the preview
        bool shouldShow = show && m_fogEnabled && m_currentTool == ToolType::FogBrush;

        // Check if we're in rectangle mode (button-controlled only)
        bool rectangleMode = m_fogRectangleModeEnabled || m_isSelectingRectangle;

        // NEVER show circle preview in rectangle mode
        if (rectangleMode) {
            shouldShow = false;
        }

        // Hide preview when panning
        if (m_isPanning) {
            shouldShow = false;
        }

        // Set visibility
        m_fogBrushPreview->setVisible(shouldShow);

        // Ensure proper Z-level when showing
        if (shouldShow) {
            m_fogBrushPreview->setZValue(1000);
            m_fogBrushPreview->update();
        }
    }
}

void MapDisplay::shareScene(MapDisplay* sourceDisplay)
{
    std::cerr << "\n=== [MapDisplay::shareScene] START ===" << std::endl;

    if (!sourceDisplay) {
        std::cerr << "[MapDisplay::shareScene] ERROR: Source display is NULL" << std::endl;
        std::cerr.flush();
        return;
    }

    std::cerr << "[MapDisplay::shareScene] Source scene: " << sourceDisplay->getScene() << std::endl;

    if (!sourceDisplay->getScene()) {
        std::cerr << "[MapDisplay::shareScene] ERROR: Source scene is NULL" << std::endl;
        std::cerr.flush();
        DebugConsole::warning("Source scene is NULL, cannot share", "Graphics");
        return;
    }

    std::cerr << "[MapDisplay::shareScene] Source scene items: "
              << sourceDisplay->getScene()->items().count() << std::endl;
    std::cerr << "[MapDisplay::shareScene] Source has mapItem: "
              << (sourceDisplay->m_mapItem ? "YES" : "NO") << std::endl;
    std::cerr << "[MapDisplay::shareScene] Source has currentMap: "
              << (!sourceDisplay->m_currentMap.isNull() ? "YES" : "NO") << std::endl;
    std::cerr.flush();

    if (m_zoomAnimation) {
        m_zoomAnimation->stop();
    }
    if (m_smoothPanTimer) {
        m_smoothPanTimer->stop();
    }
    if (m_zoomAccumulationTimer) {
        m_zoomAccumulationTimer->stop();
    }

    m_isZoomAnimating = false;

    std::cerr << "[MapDisplay::shareScene] Setting m_ownScene = false" << std::endl;
    m_ownScene = false;
    m_scene = sourceDisplay->getScene();
    std::cerr << "[MapDisplay::shareScene] Assigned scene pointer: " << m_scene << std::endl;

    if (m_scene) {
        std::cerr << "[MapDisplay::shareScene] Calling setScene()..." << std::endl;
        std::cerr.flush();
        setScene(m_scene);
        std::cerr << "[MapDisplay::shareScene] setScene() complete, items visible: "
                  << m_scene->items().count() << std::endl;
    }

    if (sourceDisplay->m_mapItem) {
        std::cerr << "[MapDisplay::shareScene] Sharing mapItem pointer" << std::endl;
        m_mapItem = sourceDisplay->m_mapItem;
    } else {
        std::cerr << "[MapDisplay::shareScene] WARNING: Source mapItem is NULL" << std::endl;
    }

    if (!sourceDisplay->m_currentMap.isNull()) {
        std::cerr << "[MapDisplay::shareScene] Sharing currentMap image" << std::endl;
        m_currentMap = sourceDisplay->m_currentMap;
    } else {
        std::cerr << "[MapDisplay::shareScene] WARNING: Source currentMap is NULL" << std::endl;
    }
    std::cerr.flush();

    if (sourceDisplay->m_gridOverlay) {
        m_gridOverlay = sourceDisplay->m_gridOverlay;
    }
    if (sourceDisplay->m_fogOverlay) {
        m_fogOverlay = sourceDisplay->m_fogOverlay;
    }
    if (sourceDisplay->m_wallSystem) {
        m_wallSystem = sourceDisplay->m_wallSystem;
    }
    if (sourceDisplay->m_portalSystem) {
        m_portalSystem = sourceDisplay->m_portalSystem;
    }
    if (sourceDisplay->m_lightingOverlay) {
        m_lightingOverlay = sourceDisplay->m_lightingOverlay;
    }
    m_gridEnabled = sourceDisplay->m_gridEnabled;
    m_fogEnabled = sourceDisplay->m_fogEnabled;
    m_wallsEnabled = sourceDisplay->m_wallsEnabled;
    m_portalsEnabled = sourceDisplay->m_portalsEnabled;

    m_zoomFactor = sourceDisplay->m_zoomFactor;
    m_targetZoomFactor = m_zoomFactor;

    // Apply the initial transform to match the source display
    resetTransform();
    scale(m_zoomFactor, m_zoomFactor);

    if (m_mapItem) {
        std::cerr << "[MapDisplay::shareScene] Making mapItem visible and updating" << std::endl;
        m_mapItem->setVisible(true);
        m_mapItem->update();
    }

    if (m_scene) {
        std::cerr << "[MapDisplay::shareScene] Updating scene" << std::endl;
        m_scene->update();
    }

    std::cerr << "[MapDisplay::shareScene] Calling update() on view" << std::endl;
    update();

    std::cerr << "=== [MapDisplay::shareScene] END ===" << std::endl;
    std::cerr.flush();
}

void MapDisplay::updateSharedScene()
{
    std::cerr << "\n=== [MapDisplay::updateSharedScene] START ===" << std::endl;
    std::cerr << "[MapDisplay::updateSharedScene] m_ownScene: " << m_ownScene
              << ", m_scene: " << m_scene << std::endl;

    if (!m_ownScene && m_scene) {
        std::cerr << "[MapDisplay::updateSharedScene] Shared scene detected, updating..." << std::endl;
        std::cerr << "[MapDisplay::updateSharedScene] Scene items before: "
                  << m_scene->items().count() << std::endl;
        std::cerr.flush();

        QMutexLocker locker(&s_sceneMutex);

        std::cerr << "[MapDisplay::updateSharedScene] Invalidating scene..." << std::endl;
        m_scene->invalidate();
        m_scene->update(m_scene->sceneRect());

        std::cerr << "[MapDisplay::updateSharedScene] Clearing and resetting scene pointer..." << std::endl;
        setScene(nullptr);
        setScene(m_scene);

        std::cerr << "[MapDisplay::updateSharedScene] Scene items after reset: "
                  << m_scene->items().count() << std::endl;

        if (viewport()) {
            std::cerr << "[MapDisplay::updateSharedScene] Updating viewport..." << std::endl;
            viewport()->update();
        }

        std::cerr << "[MapDisplay::updateSharedScene] Calling update()..." << std::endl;
        update();
    } else {
        std::cerr << "[MapDisplay::updateSharedScene] Skipping - not a shared scene" << std::endl;
    }

    std::cerr << "=== [MapDisplay::updateSharedScene] END ===" << std::endl;
    std::cerr.flush();
}

void MapDisplay::copyMapFrom(MapDisplay* sourceDisplay)
{
    std::cerr << "\n=== [MapDisplay::copyMapFrom] START - NEW ARCHITECTURE ===" << std::endl;

    if (!sourceDisplay) {
        std::cerr << "[MapDisplay::copyMapFrom] ERROR: Source display is NULL" << std::endl;
        std::cerr.flush();
        return;
    }

    QImage sourceImage = sourceDisplay->getCurrentMapImage();
    if (sourceImage.isNull()) {
        std::cerr << "[MapDisplay::copyMapFrom] ERROR: Source image is NULL" << std::endl;
        std::cerr.flush();
        return;
    }

    std::cerr << "[MapDisplay::copyMapFrom] Source image size: " << sourceImage.width() << "x" << sourceImage.height() << std::endl;
    std::cerr << "[MapDisplay::copyMapFrom] THIS owns scene: " << m_ownScene << std::endl;
    std::cerr.flush();

    QMutexLocker locker(&s_sceneMutex);

    m_currentMap = sourceImage;

    if (m_mapItem) {
        std::cerr << "[MapDisplay::copyMapFrom] Updating existing mapItem with new pixmap" << std::endl;
        m_mapItem->setPixmap(QPixmap::fromImage(m_currentMap));
    } else {
        std::cerr << "[MapDisplay::copyMapFrom] Creating NEW mapItem" << std::endl;
        m_mapItem = new QGraphicsPixmapItem(QPixmap::fromImage(m_currentMap));
        m_mapItem->setTransformationMode(Qt::SmoothTransformation);
        m_mapItem->setZValue(-1);

        if (m_scene) {
            std::cerr << "[MapDisplay::copyMapFrom] Adding mapItem to scene" << std::endl;
            m_scene->addItem(m_mapItem);
        }
    }

    if (m_scene && m_mapItem) {
        QRectF imageRect = m_mapItem->boundingRect();
        m_scene->setSceneRect(imageRect);
        std::cerr << "[MapDisplay::copyMapFrom] Scene rect set to: " << imageRect.width() << "x" << imageRect.height() << std::endl;
    }

    m_zoomFactor = sourceDisplay->m_zoomFactor;
    m_targetZoomFactor = m_zoomFactor;
    resetTransform();
    scale(m_zoomFactor, m_zoomFactor);

    std::cerr << "[MapDisplay::copyMapFrom] Calling update() on scene and view" << std::endl;
    if (m_scene) {
        m_scene->update();
    }
    update();

    std::cerr << "=== [MapDisplay::copyMapFrom] END - Map copied successfully ===" << std::endl;
    std::cerr.flush();
}

void MapDisplay::setGridEnabled(bool enabled)
{
    m_gridEnabled = enabled;
    updateGrid();
}

void MapDisplay::setFogEnabled(bool enabled)
{
    const bool stateChanged = (m_fogEnabled != enabled);
    m_fogEnabled = enabled;

    // CRITICAL FIX: When enabling fog for first time, fill entire map with black fog
    // This ensures player sees fully black screen until areas are revealed
    if (enabled && stateChanged && m_fogOverlay) {
        // Get current fog state
        const QImage& currentFog = m_fogOverlay->getFogMask();

        // Check if fog needs initialization (all transparent = never revealed anything)
        bool needsInitialFill = currentFog.isNull();

        if (!needsInitialFill && !currentFog.isNull()) {
            // Sample a few pixels to see if fog is completely transparent (uninitialized)
            bool allTransparent = true;
            int sampleSize = qMin(10, qMin(currentFog.width(), currentFog.height()));

            for (int y = 0; y < sampleSize && allTransparent; ++y) {
                for (int x = 0; x < sampleSize && allTransparent; ++x) {
                    if (qAlpha(currentFog.pixel(x, y)) > 0) {
                        allTransparent = false;
                    }
                }
            }

            needsInitialFill = allTransparent;
        }

        // Fill fog if this is first time being enabled
        if (needsInitialFill) {
            m_fogOverlay->fillAll();
            DebugConsole::info("Initialized fog with full coverage (player will see black)", "MapDisplay");
        }
    }

    updateFog();
    updateToolCursor();

    // Hide brush preview when fog is disabled
    if (!enabled) {
        showFogBrushPreview(false);
    }

    if (stateChanged) {
        // Full fog toggle requires full scene update
        if (m_fogOverlay) {
            notifyFogChanged(m_fogOverlay->boundingRect());
        }
    }
}

void MapDisplay::notifyFogChanged(const QRectF& dirtyRegion)
{
    // PRIORITY 4 FIX: Use bounded updates for 60 FPS performance
    if (m_fogOverlay) {
        m_fogOverlay->forceImmediateUpdate();
    }

    // Handle empty dirty region (full update needed for clearAll/fillAll/resetFog)
    if (dirtyRegion.isEmpty()) {
        // Full update required
        if (m_scene) {
            m_scene->update();
        }
        update();
        if (viewport()) {
            viewport()->update();
        }
    } else {
        // Bounded update for specific dirty region
        if (m_scene) {
            m_scene->update(dirtyRegion);
        }

        // Transform dirty region to view coordinates
        QRectF viewRect = mapFromScene(dirtyRegion).boundingRect();
        if (!viewRect.isEmpty()) {
            update(viewRect.toRect());
            if (viewport()) {
                viewport()->update(viewRect.toRect());
            }
        }
    }

    // Emit signal to notify that fog has changed
    emit fogChanged();
}




void MapDisplay::clearFog()
{
    if (m_fogOverlay) {
        m_fogOverlay->clearAll();
        update();
        notifyFogChanged();
    }
}

void MapDisplay::resetFog()
{
    if (m_fogOverlay) {
        m_fogOverlay->resetFog();
        update();
        notifyFogChanged();
    }
}

void MapDisplay::setWallsEnabled(bool enabled)
{
    m_wallsEnabled = enabled;
    if (m_wallSystem) {
        if (enabled) {
            if (!m_scene->items().contains(m_wallSystem)) {
                // THREAD SAFETY: Protect scene modification
                {
                    QMutexLocker locker(&s_sceneMutex);
                    m_scene->addItem(m_wallSystem);
                    m_wallSystem->setZValue(5);
                }
            }
            m_wallSystem->setVisible(true);
        } else {
            m_wallSystem->setVisible(false);
        }
    }
}

void MapDisplay::setWallDebugRenderingEnabled(bool enabled)
{
    if (m_wallSystem) {
        m_wallSystem->setDebugRenderingEnabled(enabled);
    }
}

bool MapDisplay::isWallDebugRenderingEnabled() const
{
    return m_wallSystem ? m_wallSystem->isDebugRenderingEnabled() : false;
}

void MapDisplay::setPortalsEnabled(bool enabled)
{
    m_portalsEnabled = enabled;
    if (m_portalSystem) {
        if (enabled) {
            if (!m_scene->items().contains(m_portalSystem)) {
                // THREAD SAFETY: Protect scene modification
                {
                    QMutexLocker locker(&s_sceneMutex);
                    m_scene->addItem(m_portalSystem);
                    m_portalSystem->setZValue(10);
                }
            }
            m_portalSystem->setVisible(true);
        } else {
            m_portalSystem->setVisible(false);
        }
    }
}

void MapDisplay::togglePortalAt(const QPointF& scenePos)
{
    if (m_portalSystem && m_portalsEnabled) {
        bool toggled = m_portalSystem->togglePortalAt(scenePos);
        if (toggled) {
            DebugConsole::info(QString("Portal toggled at scene position: (%1, %2)").arg(scenePos.x()).arg(scenePos.y()), "Graphics");
        }
    }
}

void MapDisplay::setFogBrushSize(int size)
{
    m_fogBrushSize = qBound(10, size, 400);  // Doubled maximum size from 200 to 400
    // Always update cursor when brush size changes, regardless of fog state
    updateFogBrushCursor();

    // Update preview if visible
    if (m_fogBrushPreview && m_fogBrushPreview->isVisible()) {
        QPointF currentPos = mapToScene(mapFromGlobal(QCursor::pos()));
        updateFogBrushPreview(currentPos);
    }
}

void MapDisplay::setFogHideModeEnabled(bool enabled)
{
    m_fogHideModeEnabled = enabled;
    updateToolCursor();

    // Update preview color based on new mode
    if (m_fogBrushPreview && m_fogBrushPreview->isVisible()) {
        QPointF currentPos = mapToScene(mapFromGlobal(QCursor::pos()));
        updateFogBrushPreview(currentPos);
    }
}

void MapDisplay::setFogRectangleModeEnabled(bool enabled)
{
    m_fogRectangleModeEnabled = enabled;

    // Clear any active rectangle selection when disabling
    if (!enabled && m_isSelectingRectangle) {
        m_isSelectingRectangle = false;
        if (m_selectionRectIndicator) {
            // THREAD SAFETY: Protect scene modification
            {
                QMutexLocker locker(&s_sceneMutex);
                m_scene->removeItem(m_selectionRectIndicator);
            }
            delete m_selectionRectIndicator;
            m_selectionRectIndicator = nullptr;
        }
    }

    updateToolCursor();

    // Hide brush preview in rectangle mode
    if (enabled) {
        showFogBrushPreview(false);
    }
}

int MapDisplay::getGridSize() const
{
    if (m_gridOverlay) {
        return m_gridOverlay->getGridSize();
    }
    return 50;
}

void MapDisplay::updateGrid()
{
    if (!m_gridOverlay || !m_scene) {
        return;
    }

    // THREAD SAFETY: Protect scene modification
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

void MapDisplay::updateFog()
{
    if (!m_fogOverlay || !m_scene) {
        return;
    }

    // THREAD SAFETY: Protect scene modification
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

void MapDisplay::fitMapToView()
{
    if (m_mapItem) {
        fitInView(m_mapItem, Qt::KeepAspectRatio);
        m_zoomFactor = transform().m11();
        m_targetZoomFactor = m_zoomFactor;

        emit zoomChanged(m_zoomFactor);
    }
}

void MapDisplay::setInitialZoom()
{
    if (!m_mapItem || m_currentMap.isNull()) {
        return;
    }

    // Always fit the entire image to the screen when loading
    // This ensures the full map is immediately visible on any resolution
    fitMapToView();
}

void MapDisplay::setZoomLevel(qreal zoomLevel)
{
    if (!m_mapItem) {
        return;
    }

    // Clamp zoom level
    zoomLevel = qBound(MIN_ZOOM, zoomLevel, MAX_ZOOM);

    // Reset transform and apply new zoom
    resetTransform();
    scale(zoomLevel, zoomLevel);

    m_zoomFactor = zoomLevel;
    m_targetZoomFactor = zoomLevel;

    // Show zoom indicator
    if (m_zoomIndicator) {
        m_zoomIndicator->showZoom(m_zoomFactor);
    }

    emit zoomChanged(m_zoomFactor);
}

void MapDisplay::zoomToPreset(qreal zoomLevel)
{
    if (!m_mapItem) {
        return;
    }

    // Disable animation on extremely large maps to avoid stutter
    const QSizeF mapSize = m_scene ? m_scene->sceneRect().size() : QSizeF();
    const qreal mapArea = mapSize.width() * mapSize.height();
    if (mapArea > 2.0e7) { // ~20 megapixels threshold
        setZoomLevel(qBound(MIN_ZOOM, zoomLevel, MAX_ZOOM));
        return;
    }

    // Animate to preset zoom level
    if (m_zoomAnimation && m_zoomAnimation->state() == QAbstractAnimation::Running) {
        m_zoomAnimation->stop();
    }

    m_targetZoomFactor = qBound(MIN_ZOOM, zoomLevel, MAX_ZOOM);

    // For preset zoom, do not center on cursor during animation
    m_zoomCenterOnCursor = false;

    // Configure and start the existing animation instance
    m_zoomAnimation->setDuration(250);
    m_zoomAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    m_zoomAnimation->setStartValue(m_zoomFactor);
    m_zoomAnimation->setEndValue(m_targetZoomFactor);
    m_zoomAnimation->start();
}

void MapDisplay::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    // Update zoom indicator position
    if (m_zoomIndicator) {
        m_zoomIndicator->updatePosition();
    }

    // Update loading progress widget position
    if (m_loadingProgressWidget) {
        m_loadingProgressWidget->updatePosition();
    }

    // Keep OpenGL overlay in sync with view size
    if (m_openglDisplay) {
        m_openglDisplay->resize(size());
        m_openglDisplay->move(0, 0);
        // CRITICAL FIX: Don't raise OpenGL - keep it in background
        m_openglDisplay->lower();
    }
}

void MapDisplay::wheelEvent(QWheelEvent *event)
{
    if (!m_zoomControlsEnabled) {
        event->ignore();
        return;
    }

    // Stop momentum when user starts interacting
    if (m_smoothPanTimer->isActive()) {
        m_smoothPanTimer->stop();
        m_panVelocity = QPointF(0, 0);
    }

    const qreal zoomStep = 0.1;
    qreal currentScale = transform().m11();
    qreal newTargetZoom;

    if (event->angleDelta().y() > 0) {
        newTargetZoom = qMin(currentScale + zoomStep, MAX_ZOOM);
    } else {
        newTargetZoom = qMax(currentScale - zoomStep, MIN_ZOOM);
    }

    // Store the cursor position for zoom centering
    m_zoomAnimationCenter = event->position();
    m_zoomCenterOnCursor = true;

    // If we're already animating, accumulate the zoom change
    if (m_isZoomAnimating) {
        // Update target zoom and restart accumulation timer
        m_animationTargetZoom = newTargetZoom;
        m_zoomAccumulationTimer->start();
    } else {
        // Start new zoom animation
        m_animationStartZoom = currentScale;
        m_animationTargetZoom = newTargetZoom;
        m_isZoomAnimating = true;
        
        // Start accumulation timer to handle rapid wheel events
        m_zoomAccumulationTimer->start();
    }

    // Show zoom indicator immediately for responsiveness
    if (m_zoomIndicator) {
        m_zoomIndicator->showZoom(m_animationTargetZoom);
    }
}

void MapDisplay::mousePressEvent(QMouseEvent *event)
{
    // Stop momentum when user starts new interaction
    if (m_smoothPanTimer->isActive()) {
        m_smoothPanTimer->stop();
        m_panVelocity = QPointF(0, 0);
    }
    // Middle mouse button for panning (no modifiers)
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);

        // Initialize velocity tracking for momentum
        m_velocityHistory.clear();
        m_velocityTimestamps.clear();
        m_lastMoveTime = QDateTime::currentMSecsSinceEpoch();
        m_panVelocity = QPointF(0, 0);

        // Stop any ongoing momentum animation
        if (m_smoothPanTimer->isActive()) {
            m_smoothPanTimer->stop();
        }
    } else if (event->button() == Qt::LeftButton && (m_currentTool == ToolType::FogBrush || m_currentTool == ToolType::FogRectangle) && m_fogOverlay && m_fogEnabled) {
        QPointF scenePos = mapToScene(event->pos());
        FogToolMode mode = getCurrentFogToolMode();

        if (mode == FogToolMode::UnifiedFog) {
            // Rectangle mode controlled by button state only (no Shift key)
            if (m_fogRectangleModeEnabled) {

                // Start rectangle selection
                m_isSelectingRectangle = true;
                m_rectangleStartPos = scenePos;
                m_currentSelectionRect = QRectF(scenePos, scenePos);

                // Create visual indicator for rectangle selection
                // THREAD SAFETY: Protect scene modification
                {
                    QMutexLocker locker(&s_sceneMutex);
                    if (m_selectionRectIndicator) {
                        m_scene->removeItem(m_selectionRectIndicator);
                        delete m_selectionRectIndicator;
                    }

                    m_selectionRectIndicator = new QGraphicsRectItem(m_currentSelectionRect);
                    QPen pen(QColor(255, 255, 0), 2);  // Yellow for reveal
                    pen.setStyle(Qt::DashLine);
                    m_selectionRectIndicator->setPen(pen);
                    m_selectionRectIndicator->setBrush(Qt::NoBrush);
                    m_scene->addItem(m_selectionRectIndicator);
                }
            } else {
                // Circular fog brush - always reveal (no hide mode)
                m_fogOverlay->revealAreaFeathered(scenePos, m_fogBrushSize / 2.0);
                emit fogChanged();
            }
        }
        update();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void MapDisplay::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPoint;
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 timeDelta = currentTime - m_lastMoveTime;

        // Apply the pan movement
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());

        // Track velocity for momentum (only if enough time has passed)
        if (timeDelta > 5) { // Minimum 5ms between samples to avoid division by zero
            QPointF velocity = QPointF(delta.x(), delta.y()) / timeDelta * 16.0; // Scale to 60 FPS

            // Keep a rolling window of recent velocity samples
            m_velocityHistory.append(velocity);
            m_velocityTimestamps.append(currentTime);

            // Keep only the last 150ms of history for smooth momentum calculation
            while (!m_velocityTimestamps.isEmpty() &&
                   (currentTime - m_velocityTimestamps.first()) > 150) {
                m_velocityHistory.removeFirst();
                m_velocityTimestamps.removeFirst();
            }

            m_lastMoveTime = currentTime;
        }

        m_lastPanPoint = event->pos();
    } else if (m_isSelectingRectangle && m_selectionRectIndicator) {
        // Update rectangle selection
        QPointF scenePos = mapToScene(event->pos());
        m_currentSelectionRect = QRectF(m_rectangleStartPos, scenePos).normalized();
        m_selectionRectIndicator->setRect(m_currentSelectionRect);
        update();
    } else if (event->buttons() & Qt::LeftButton && m_currentTool == ToolType::FogBrush && m_fogOverlay && m_fogEnabled) {
        FogToolMode mode = getCurrentFogToolMode();

        // Brush drag - always reveal (no hide mode)
        if (mode == FogToolMode::UnifiedFog && !m_isSelectingRectangle) {
            QPointF scenePos = mapToScene(event->pos());
            m_fogOverlay->revealAreaFeathered(scenePos, m_fogBrushSize / 2.0);
            emit fogChanged();
            update();
        }
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }

    // Update fog brush preview and cursor if fog tools are active
    if (m_currentTool == ToolType::FogBrush && m_fogEnabled) {
        bool rectangleMode = m_fogRectangleModeEnabled || m_isSelectingRectangle;

        // Always update cursor to reflect current mode
        updateToolCursor();

        // Update preview position and visibility
        QPointF scenePos = mapToScene(event->pos());

        if (!rectangleMode && !m_isPanning) {
            // Circle mode: Show preview and update position
            updateFogBrushPreview(scenePos, Qt::NoModifier);
            showFogBrushPreview(true);
        } else {
            // Rectangle mode or panning: Hide preview
            showFogBrushPreview(false);
        }
    } else {
        // Not in fog brush mode: ensure preview is hidden
        showFogBrushPreview(false);
    }
}

void MapDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && m_isPanning)) {
        m_isPanning = false;
        updateToolCursor();

        // Calculate release velocity from recent movement history
        calculateReleaseVelocity();

        // Start momentum animation if velocity is significant
        if (m_panVelocity.manhattanLength() > 2.0) {
            m_smoothPanTimer->start();
        }
    } else if (event->button() == Qt::LeftButton && m_isSelectingRectangle && m_fogOverlay) {
        // Complete rectangle selection
        m_isSelectingRectangle = false;

        // Apply fog operation to the selected rectangle (always reveal)
        if (!m_currentSelectionRect.isEmpty()) {
            FogToolMode mode = getCurrentFogToolMode();
            if (mode == FogToolMode::UnifiedFog) {
                m_fogOverlay->revealRectangle(m_currentSelectionRect);
                emit fogChanged();
            }
        }

        // Clean up selection indicator
        if (m_selectionRectIndicator) {
            QMutexLocker locker(&s_sceneMutex);  // Thread-safe scene modification
            m_scene->removeItem(m_selectionRectIndicator);
            delete m_selectionRectIndicator;
            m_selectionRectIndicator = nullptr;
        }

        update();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void MapDisplay::keyPressEvent(QKeyEvent *event)
{
    // DEBUG: Log key press events for tool switching
    if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_3) {
        std::cerr << "MapDisplay::keyPressEvent - Key pressed: " << event->key()
                  << " Modifiers: " << event->modifiers() << std::endl;
        std::cerr.flush();
    }

    // Handle tool switching first (number keys without modifiers)
    if (event->modifiers() == Qt::NoModifier) {
        switch(event->key()) {
            // NO number key shortcuts per CLAUDE.md
            case Qt::Key_2:
                // Legacy handler - no longer used (remove in cleanup)
                std::cerr.flush();
                emit toolSwitchRequested(ToolType::FogBrush);
                return;
            case Qt::Key_3:
                // Pointer tool
                std::cerr << "MapDisplay: Emitting toolSwitchRequested(Pointer)" << std::endl;
                std::cerr.flush();
                emit toolSwitchRequested(ToolType::Pointer);
                return;
            default:
                break;
        }
    }

    // Zoom shortcuts
    if (m_zoomControlsEnabled) {
        bool handled = true;

        switch(event->key()) {
            case Qt::Key_Plus:
            case Qt::Key_Equal:
                // Zoom in (+/=)
                zoomToPreset(m_zoomFactor * 1.2);
                break;

            case Qt::Key_Minus:
            case Qt::Key_Underscore:
                // Zoom out (-)
                zoomToPreset(m_zoomFactor / 1.2);
                break;

            case Qt::Key_0:
                // Fit to screen
                fitMapToView();
                break;

            case Qt::Key_1:
                // 100% zoom (Ctrl+1)
                if (event->modifiers() & Qt::ControlModifier) {
                    zoomToPreset(1.0);
                } else {
                    handled = false;
                }
                break;

            case Qt::Key_2:
                // 200% zoom (Ctrl+2)
                if (event->modifiers() & Qt::ControlModifier) {
                    zoomToPreset(2.0);
                } else {
                    handled = false;
                }
                break;

            case Qt::Key_3:
                // 300% zoom (Ctrl+3)
                if (event->modifiers() & Qt::ControlModifier) {
                    zoomToPreset(3.0);
                } else {
                    handled = false;
                }
                break;

            case Qt::Key_4:
                // 50% zoom
                zoomToPreset(0.5);
                break;

            case Qt::Key_5:
                // 25% zoom
                zoomToPreset(0.25);
                break;

            case Qt::Key_6:
                // 150% zoom
                zoomToPreset(1.5);
                break;

            case Qt::Key_P:
                // Toggle nearest portal (P key - D is reserved for Draw tool)
                if (m_portalsEnabled && m_portalSystem) {
                    QPointF scenePos = mapToScene(mapFromGlobal(QCursor::pos()));
                    togglePortalAt(scenePos);
                }
                break;

            default:
                handled = false;
                break;
        }

        if (handled) {
            return;
        }
    }

    // Fog brush size adjustment with [ ] keys
    if (m_currentTool == ToolType::FogBrush && m_fogEnabled) {
        if (event->key() == Qt::Key_BracketLeft) {
            // Decrease brush size
            int newSize = qMax(10, m_fogBrushSize - 10);
            setFogBrushSize(newSize);
            showBrushSizeHUD(newSize);
            return;
        } else if (event->key() == Qt::Key_BracketRight) {
            // Increase brush size
            int newSize = qMin(200, m_fogBrushSize + 10);
            setFogBrushSize(newSize);
            showBrushSizeHUD(newSize);
            return;
        }
    }


    // Token deletion
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
        for (QGraphicsItem* item : selectedItems) {
            // Handle deletion of selected items if needed
        }
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void MapDisplay::animateZoom()
{
    if (!m_zoomAnimation) return;

    qreal currentValue = m_zoomAnimation->currentValue().toReal();
    if (currentValue <= 0) return;

    // Reset transform and apply new zoom level
    resetTransform();
    scale(currentValue, currentValue);
    m_zoomFactor = currentValue;

    if (m_zoomCenterOnCursor) {
        // Center the zoom on the cursor position
        QPointF scenePosAfterZoom = mapToScene(m_zoomCursorPos.toPoint());
        QPointF delta = scenePosAfterZoom - m_zoomScenePos;

        horizontalScrollBar()->setValue(
            horizontalScrollBar()->value() - delta.x()
        );
        verticalScrollBar()->setValue(
            verticalScrollBar()->value() - delta.y()
        );
    }

    // Emit zoom change for synchronization with player window
    emit zoomChanged(m_zoomFactor);
}

void MapDisplay::updateSmoothPan()
{
    // Slightly stronger friction for tighter momentum feel
    const qreal friction = 0.93;
    const qreal minVelocity = 0.6;

    if (m_panVelocity.manhattanLength() < minVelocity) {
        m_smoothPanTimer->stop();
        m_panVelocity = QPointF(0, 0);
        return;
    }

    // Apply velocity to scrollbars
    horizontalScrollBar()->setValue(
        horizontalScrollBar()->value() - m_panVelocity.x()
    );
    verticalScrollBar()->setValue(
        verticalScrollBar()->value() - m_panVelocity.y()
    );

    // Apply friction
    m_panVelocity *= friction;
}

void MapDisplay::calculateReleaseVelocity()
{
    // Reset velocity
    m_panVelocity = QPointF(0, 0);

    if (m_velocityHistory.isEmpty()) {
        return;
    }

    // Calculate weighted average of recent velocities
    // More recent samples have higher weight
    QPointF totalVelocity(0, 0);
    qreal totalWeight = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    for (int i = 0; i < m_velocityHistory.size(); ++i) {
        qint64 age = currentTime - m_velocityTimestamps[i];
        // Exponential weight decay - more recent = higher weight
        qreal weight = qExp(-age / 50.0); // 50ms decay constant

        totalVelocity += m_velocityHistory[i] * weight;
        totalWeight += weight;
    }

    if (totalWeight > 0) {
        m_panVelocity = totalVelocity / totalWeight;

        // Cap maximum momentum velocity for control
        const qreal maxVelocity = 30.0;
        if (m_panVelocity.manhattanLength() > maxVelocity) {
            m_panVelocity = m_panVelocity * (maxVelocity / m_panVelocity.manhattanLength());
        }
    }

    // Clear velocity history after calculation
    m_velocityHistory.clear();
    m_velocityTimestamps.clear();
}

void MapDisplay::finishZoomAccumulation()
{
    if (!m_isZoomAnimating) {
        return;
    }

    // Only animate if there's a significant change
    if (qAbs(m_animationTargetZoom - m_animationStartZoom) < 0.01) {
        m_isZoomAnimating = false;
        return;
    }

    // Stop any existing zoom animation
    if (m_zoomAnimation && m_zoomAnimation->state() == QAbstractAnimation::Running) {
        m_zoomAnimation->stop();
    }

    // Store scene position before zoom for centering
    QPointF scenePosBeforeZoom = mapToScene(m_zoomAnimationCenter.toPoint());

    // Disable animation on extremely large maps to avoid stutter
    const QSizeF mapSize = m_scene ? m_scene->sceneRect().size() : QSizeF();
    const qreal mapArea = mapSize.width() * mapSize.height();
    if (mapArea > 2.0e7) { // ~20 megapixels threshold
        setZoomLevel(m_animationTargetZoom);
        m_isZoomAnimating = false;
        return;
    }

    // Setup and start the smooth zoom animation
    m_zoomAnimation->setDuration(250); // 250ms for smooth feel
    m_zoomAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    m_zoomAnimation->setStartValue(m_animationStartZoom);
    m_zoomAnimation->setEndValue(m_animationTargetZoom);

    // Store the scene position for use during animation
    m_zoomScenePos = scenePosBeforeZoom;
    m_zoomCursorPos = m_zoomAnimationCenter;
    m_zoomCenterOnCursor = true;

    // Connect finished signal to reset animation state
    connect(m_zoomAnimation, &QPropertyAnimation::finished, this, [this]() {
        m_isZoomAnimating = false;
        m_targetZoomFactor = m_animationTargetZoom;
    }, Qt::UniqueConnection);

    m_zoomAnimation->start();
}

void MapDisplay::paintEvent(QPaintEvent *event)
{
    // Set the flag after first paint - this ensures app.exec() has started
    if (!s_appReadyForProgress) {
        s_appReadyForProgress = true;
        DebugConsole::system("App is now ready for progress events", "Graphics");
    }

    // Call base class paintEvent first
    QGraphicsView::paintEvent(event);

    // Draw empty state prompt if no map is loaded
    if (!m_mapItem) {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);

        // Get viewport dimensions
        const QRect viewRect = viewport()->rect();
        const int centerX = viewRect.width() / 2;
        const int centerY = viewRect.height() / 2;

        // Main prompt text - large and prominent
        QFont mainFont(QFont().defaultFamily(), 18);
        mainFont.setWeight(QFont::Medium);
        painter.setFont(mainFont);
        painter.setPen(QColor(176, 176, 176));  // TEXT_SECONDARY from theme

        QString mainText = "Drag & Drop Map Here";
        QFontMetrics mainMetrics(mainFont);
        QRect mainTextRect = mainMetrics.boundingRect(mainText);
        painter.drawText(centerX - mainTextRect.width() / 2,
                        centerY - 10,
                        mainText);

        // Format hint text - smaller and more subtle
        QFont hintFont(QFont().defaultFamily(), 11);
        painter.setFont(hintFont);
        painter.setPen(QColor(96, 96, 96));  // TEXT_DISABLED from theme

        QString hintText = "Supports: PNG, JPG, WebP, DD2VTT, UVTT, DF2VTT";
        QFontMetrics hintMetrics(hintFont);
        QRect hintTextRect = hintMetrics.boundingRect(hintText);
        painter.drawText(centerX - hintTextRect.width() / 2,
                        centerY + 20,
                        hintText);

        // Draw subtle dashed border
        painter.setPen(QPen(QColor(60, 60, 60), 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);

        const int margin = 80;
        QRect borderRect(margin, margin,
                        viewRect.width() - 2 * margin,
                        viewRect.height() - 2 * margin);
        painter.drawRoundedRect(borderRect, 12, 12);

        // Draw small drop icon above main text
        painter.setPen(QColor(96, 96, 96));
        painter.setFont(QFont(QFont().defaultFamily(), 32));
        QString iconText = "⬇";
        QFontMetrics iconMetrics(painter.font());
        QRect iconRect = iconMetrics.boundingRect(iconText);
        painter.drawText(centerX - iconRect.width() / 2,
                        centerY - 45,
                        iconText);
    }
}

QByteArray MapDisplay::saveFogState() const
{
    if (!m_fogOverlay) {
        return QByteArray();
    }
    
    return m_fogOverlay->saveState();
}

bool MapDisplay::loadFogState(const QByteArray& data)
{
    if (!m_fogOverlay || data.isEmpty()) {
        return false;
    }
    
    return m_fogOverlay->loadState(data);
}

void MapDisplay::connectFogChanges(QObject* receiver, const char* slot)
{
    if (receiver) {
        connect(this, SIGNAL(fogChanged()), receiver, slot);
    }
}


void MapDisplay::syncZoomLevel(qreal zoomLevel, const QPointF& centerPoint)
{
    if (qAbs(m_zoomFactor - zoomLevel) < 0.01) {
        return; // Already at the target zoom level
    }

    // Reset transform and apply new zoom level directly
    resetTransform();
    scale(zoomLevel, zoomLevel);
    m_zoomFactor = zoomLevel;
    m_targetZoomFactor = zoomLevel;

    // Center on the specified point if provided
    if (!centerPoint.isNull()) {
        centerOn(centerPoint);
    }

    // Update the view
    update();

    // NOTE: Do not emit zoomChanged here to avoid feedback loop
    // This method is for receiving zoom updates, not broadcasting them
}



void MapDisplay::createPing(const QPointF& scenePos)
{
    if (!m_scene) {
        return;
    }

    // THREAD SAFETY: Protect scene modification
    {
        QMutexLocker locker(&s_sceneMutex);
        PingIndicator* ping = new PingIndicator(scenePos);
        m_scene->addItem(ping);
        ping->startAnimation();
    }
}

void MapDisplay::createGMBeacon(const QPointF& scenePos)
{
    if (!m_scene) {
        return;
    }

    // Calculate viewport width for relative sizing
    qreal viewportWidth = viewport()->width();
    // THREAD SAFETY: Protect scene modification
    {
        QMutexLocker locker(&s_sceneMutex);
        GMBeacon* beacon = new GMBeacon(scenePos, viewportWidth);
        m_scene->addItem(beacon);
    }
}

void MapDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());

        // ALWAYS create GM beacon on double-click (global behavior)
        // Simplified UX: Double-click works in ANY mode for player attention
        createGMBeacon(scenePos);
        event->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

// Lighting Effects implementation with lazy loading
LightingOverlay* MapDisplay::getLightingOverlay()
{
    if (!m_lightingOverlay) {
        // THREAD SAFETY: Protect scene modification
        QMutexLocker locker(&s_sceneMutex);
        // Double-check after acquiring lock (double-checked locking pattern)
        if (!m_lightingOverlay) {
            m_lightingOverlay = new LightingOverlay();
            m_scene->addItem(m_lightingOverlay);
            m_lightingOverlay->setZValue(600);
            // Keep the default enabled state from LightingOverlay constructor
            // which is true - this ensures menu state matches actual state
        }
    }
    return m_lightingOverlay;
}

// Lighting system implementation
void MapDisplay::setLightingEnabled(bool enabled)
{
    getLightingOverlay()->setEnabled(enabled);
}

void MapDisplay::setTimeOfDay(int timeOfDay)
{
    getLightingOverlay()->setTimeOfDay(static_cast<TimeOfDay>(timeOfDay));
}

void MapDisplay::setLightingIntensity(qreal intensity)
{
    getLightingOverlay()->setLightingIntensity(intensity);
}

void MapDisplay::setCustomLightingTint(const QColor& tint)
{
    getLightingOverlay()->setLightingTint(tint);
}

void MapDisplay::applyVTTLighting(bool globalLight, qreal darkness)
{
    getLightingOverlay()->setGlobalLight(globalLight);
    getLightingOverlay()->setDarkness(darkness);
}

bool MapDisplay::isLightingEnabled() const
{
    return m_lightingOverlay && m_lightingOverlay->isEnabled();
}

int MapDisplay::getTimeOfDay() const
{
    if (m_lightingOverlay) {
        return static_cast<int>(m_lightingOverlay->getTimeOfDay());
    }
    return 1; // Default to Day
}

// Point light management - Feature removed
void MapDisplay::setPointLightPlacementMode(bool enabled)
{
    Q_UNUSED(enabled);
    // Point light feature removed
}

void MapDisplay::addPointLight(const QPointF& position)
{
    Q_UNUSED(position);
    // Point light feature removed
}

void MapDisplay::removePointLight(const QUuid& lightId)
{
    Q_UNUSED(lightId);
    // Point light feature removed
}

void MapDisplay::clearAllPointLights()
{
    // Point light feature removed
}

void MapDisplay::setAmbientLightLevel(qreal level)
{
    getLightingOverlay()->setAmbientLightLevel(level);
}

qreal MapDisplay::getAmbientLightLevel() const
{
    if (m_lightingOverlay) {
        return m_lightingOverlay->getAmbientLightLevel();
    }
    return 0.2; // Default ambient level
}

FogToolMode MapDisplay::getCurrentFogToolMode() const
{
    if (m_mainWindow) {
        return m_mainWindow->getFogToolMode();
    }
    // Default fallback if no main window reference
    return FogToolMode::UnifiedFog;
}
    

void MapDisplay::updateToolCursor()
{
    updateCursor(m_currentTool, m_isDraggingTool || m_isPanning);
}

void MapDisplay::updateFogBrushCursor()
{
    updateFogBrushCursor(m_fogBrushSize, m_currentFogMode);
}


// OpenGL integration methods
void MapDisplay::setOpenGLRenderingEnabled(bool enabled)
{
    if (m_openglRenderingEnabled == enabled) {
        return;
    }

    m_openglRenderingEnabled = enabled;

    if (enabled) {
        // Create OpenGL display if it doesn't exist
        if (!m_openglDisplay) {
            m_openglDisplay = new OpenGLMapDisplay(this);

            // Position the OpenGL widget to overlay the graphics view
            m_openglDisplay->resize(size());
            m_openglDisplay->move(0, 0);
            
            // CRITICAL FIX: Set proper stacking order - OpenGL should be background layer
            m_openglDisplay->lower(); // Move to back
            
            // Make OpenGL widget semi-transparent to allow Qt content to show through
            m_openglDisplay->setAttribute(Qt::WA_AlwaysStackOnTop, false);
            m_openglDisplay->setAttribute(Qt::WA_OpaquePaintEvent, false);

            // Load current map into OpenGL display if available
            if (!m_currentMap.isNull()) {
                m_openglDisplay->loadTexture(m_currentMap);
            }

            // Sync lighting state
            if (m_lightingOverlay) {
                m_openglDisplay->setLightingEnabled(isLightingEnabled());
                m_openglDisplay->setTimeOfDay(getTimeOfDay());
                m_openglDisplay->setAmbientLightLevel(getAmbientLightLevel());
            }
        }

        // CRITICAL FIX: Always show Qt Graphics View content first, OpenGL as enhancement
        // Don't hide the Qt content when OpenGL is enabled
        if (!m_currentMap.isNull()) {
            m_openglDisplay->show();
            m_openglDisplay->lower(); // Keep in background
        } else {
            m_openglDisplay->hide();
        }

        DebugConsole::system("OpenGL rendering enabled for MapDisplay", "OpenGL");
    } else {
        // Hide OpenGL display, fall back to QPainter
        if (m_openglDisplay) {
            m_openglDisplay->hide();
        }

        DebugConsole::warning("OpenGL rendering disabled, using QPainter fallback", "OpenGL");
    }

    update();
}

void MapDisplay::forceOpenGLRefresh()
{
    // CRITICAL FIX: Force complete OpenGL refresh to recover from black screen
    if (!m_openglRenderingEnabled || !m_openglDisplay) {
        return;
    }

    // Reload texture if we have a current map
    if (!m_currentMap.isNull()) {
        m_openglDisplay->loadTexture(m_currentMap);
        m_openglDisplay->update();
    }

    // Ensure OpenGL widget is visible and properly positioned
    m_openglDisplay->resize(size());
    m_openglDisplay->move(0, 0);
    m_openglDisplay->show();
    m_openglDisplay->lower(); // Keep in background

    DebugConsole::info("Forced OpenGL refresh to recover from display issue", "OpenGL");
}

// Weather effects - Feature removed
void MapDisplay::setWeatherType(int weatherType)
{
    Q_UNUSED(weatherType);
    // Weather feature removed
}

int MapDisplay::getWeatherType() const
{
    return 0; // None - weather removed
}

void MapDisplay::setWeatherIntensity(float intensity)
{
    Q_UNUSED(intensity);
    // Weather feature removed
}

float MapDisplay::getWeatherIntensity() const
{
    return 0.5f; // Default - weather removed
}

void MapDisplay::setWindDirection(float x, float y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    // Weather feature removed
}

void MapDisplay::setWindStrength(float strength)
{
    Q_UNUSED(strength);
    // Weather feature removed
}

// Post-processing effects delegation methods
void MapDisplay::setBloomEnabled(bool enabled)
{
    if (m_openglDisplay) {
        m_openglDisplay->setBloomEnabled(enabled);
    }
}

void MapDisplay::setBloomThreshold(float threshold)
{
    if (m_openglDisplay) {
        m_openglDisplay->setBloomThreshold(threshold);
    }
}

void MapDisplay::setBloomIntensity(float intensity)
{
    if (m_openglDisplay) {
        m_openglDisplay->setBloomIntensity(intensity);
    }
}

void MapDisplay::setBloomRadius(float radius)
{
    if (m_openglDisplay) {
        m_openglDisplay->setBloomRadius(radius);
    }
}

void MapDisplay::setShadowMappingEnabled(bool enabled)
{
    if (m_openglDisplay) {
        m_openglDisplay->setShadowMappingEnabled(enabled);
    }
}

void MapDisplay::setShadowMapSize(int size)
{
    if (m_openglDisplay) {
        m_openglDisplay->setShadowMapSize(size);
    }
}

void MapDisplay::setVolumetricLightingEnabled(bool enabled)
{
    if (m_openglDisplay) {
        m_openglDisplay->setVolumetricLightingEnabled(enabled);
    }
}

void MapDisplay::setVolumetricIntensity(float intensity)
{
    if (m_openglDisplay) {
        m_openglDisplay->setVolumetricIntensity(intensity);
    }
}

void MapDisplay::setLightShaftsEnabled(bool enabled)
{
    if (m_openglDisplay) {
        m_openglDisplay->setLightShaftsEnabled(enabled);
    }
}

void MapDisplay::setLightShaftsIntensity(float intensity)
{
    if (m_openglDisplay) {
        m_openglDisplay->setLightShaftsIntensity(intensity);
    }
}

void MapDisplay::setMSAAEnabled(bool enabled)
{
    if (m_openglDisplay) {
        m_openglDisplay->setMSAAEnabled(enabled);
    }
}

void MapDisplay::setMSAASamples(int samples)
{
    if (m_openglDisplay) {
        m_openglDisplay->setMSAASamples(samples);
    }
}

// Post-processing getters delegation methods
bool MapDisplay::isBloomEnabled() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->isBloomEnabled();
    }
    return false;
}

bool MapDisplay::isShadowMappingEnabled() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->isShadowMappingEnabled();
    }
    return false;
}

bool MapDisplay::isVolumetricLightingEnabled() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->isVolumetricLightingEnabled();
    }
    return false;
}

bool MapDisplay::isLightShaftsEnabled() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->isLightShaftsEnabled();
    }
    return false;
}

bool MapDisplay::isMSAAEnabled() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->isMSAAEnabled();
    }
    return false;
}

float MapDisplay::getBloomThreshold() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->getBloomThreshold();
    }
    return 0.8f;
}

float MapDisplay::getBloomIntensity() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->getBloomIntensity();
    }
    return 1.0f;
}

float MapDisplay::getBloomRadius() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->getBloomRadius();
    }
    return 1.0f;
}

int MapDisplay::getShadowMapSize() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->getShadowMapSize();
    }
    return 2048;
}

float MapDisplay::getVolumetricIntensity() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->getVolumetricIntensity();
    }
    return 0.5f;
}

float MapDisplay::getLightShaftsIntensity() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->getLightShaftsIntensity();
    }
    return 0.5f;
}

int MapDisplay::getMSAASamples() const
{
    if (m_openglDisplay) {
        return m_openglDisplay->getMSAASamples();
    }
    return 4;
}

void MapDisplay::setActiveTool(ToolType tool, bool isDragging)
{
    m_currentTool = tool;
    m_isDraggingTool = isDragging;
    updateCursor(tool, isDragging);
}

void MapDisplay::updateCursor(ToolType tool, bool isDragging)
{
    QCursor newCursor;

    switch (tool) {
        case ToolType::Pointer:
            newCursor = CustomCursors::createPointerCursor();
            break;

        case ToolType::FogBrush:
            {
                // Check if we're in rectangle mode
                Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
                bool rectangleMode = m_fogRectangleModeEnabled || (modifiers & Qt::ShiftModifier) || m_isSelectingRectangle;

                if (rectangleMode) {
                    // Rectangle mode: Use crosshair cursor, no circle preview
                    newCursor = QCursor(Qt::CrossCursor);
                    // Ensure circle preview is hidden in rectangle mode
                    if (m_fogBrushPreview) {
                        m_fogBrushPreview->setVisible(false);
                    }
                } else {
                    // Circle mode: Use crosshair cursor so user can see exact position
                    // The circle preview shows the brush area
                    newCursor = QCursor(Qt::CrossCursor);
                    // Circle preview will be shown by mouseMoveEvent
                }
            }
            break;

        case ToolType::FogRectangle:
            newCursor = CustomCursors::createRectangleCursor();
            break;

        default:
            newCursor = QCursor(Qt::ArrowCursor);
            break;
    }

    setCursor(newCursor);
}

void MapDisplay::updateFogBrushCursor(int brushSize, FogToolMode mode)
{
    // This function is deprecated - cursor is now managed by updateCursor()
    // Keeping empty implementation for compatibility
    Q_UNUSED(brushSize);
    Q_UNUSED(mode);
}

void MapDisplay::onToolChanged(ToolType tool)
{
    m_currentTool = tool;
    m_isDraggingTool = false;
    updateCursor(tool, false);

    // Handle tool-specific setup
    switch (tool) {
        case ToolType::FogBrush:
            // Auto-enable fog when fog brush tool is selected
            if (!m_fogEnabled) {
                setFogEnabled(true);
                DebugConsole::info("Auto-enabled fog for fog brush tool", "MapDisplay");
            }
            // Disable rectangle mode when switching to brush
            setFogRectangleModeEnabled(false);
            // Show fog brush preview
            if (m_fogEnabled) {
                showFogBrushPreview(true);
            }
            break;

        case ToolType::FogRectangle:
            // Auto-enable fog when fog rectangle tool is selected
            if (!m_fogEnabled) {
                setFogEnabled(true);
                DebugConsole::info("Auto-enabled fog for fog rectangle tool", "MapDisplay");
            }
            // Enable rectangle mode for drag-to-reveal
            setFogRectangleModeEnabled(true);
            // Hide fog brush preview in rectangle mode
            showFogBrushPreview(false);
            break;

        case ToolType::Pointer:
        default:
            // Disable fog-specific modes when switching away
            setFogRectangleModeEnabled(false);
            showFogBrushPreview(false);
            break;
    }

    DebugConsole::info(
        QString("MapDisplay cursor updated for tool: %1")
        .arg(static_cast<int>(tool)), "MapDisplay");
}

void MapDisplay::onFogToolModeChanged(FogToolMode mode)
{
    m_currentFogMode = mode;

    if (m_currentTool == ToolType::FogBrush) {
        // Update cursor to reflect new mode
        updateToolCursor();
        // Update preview if visible
        if (m_fogBrushPreview && m_fogBrushPreview->isVisible()) {
            QPointF currentPos = mapToScene(mapFromGlobal(QCursor::pos()));
            updateFogBrushPreview(currentPos);
        }
    }

    DebugConsole::info(
        QString("MapDisplay fog mode changed to: %1")
        .arg(static_cast<int>(mode)), "MapDisplay");
}

void MapDisplay::onFogBrushSizeChanged(int size)
{
    m_fogBrushSize = size;
    if (m_currentTool == ToolType::FogBrush) {
        // Update cursor to reflect new size
        updateToolCursor();
        // Update preview if visible
        if (m_fogBrushPreview && m_fogBrushPreview->isVisible()) {
            QPointF currentPos = mapToScene(mapFromGlobal(QCursor::pos()));
            updateFogBrushPreview(currentPos);
        }
    }

    DebugConsole::info(
        QString("MapDisplay fog cursor updated for size: %1")
        .arg(size), "MapDisplay");
}

void MapDisplay::showBrushSizeHUD(int size)
{
    if (!m_brushSizeHUD) return;

    m_brushSizeHUD->setText(QString("Brush Size: %1px").arg(size));

    // Position HUD in the center-top of the viewport
    QRect viewportRect = viewport()->rect();
    m_brushSizeHUD->adjustSize();
    int hudWidth = m_brushSizeHUD->width();
    int hudHeight = m_brushSizeHUD->height();

    int x = (viewportRect.width() - hudWidth) / 2;
    int y = 40; // 40px from top

    m_brushSizeHUD->move(x, y);

    // Use smooth fade-in animation for premium feel
    if (!m_brushSizeHUD->isVisible()) {
        AnimationHelper::fadeIn(m_brushSizeHUD, AnimationHelper::STANDARD_DURATION);
    }

    // Reset fade timer
    m_hudFadeTimer->start();
}

void MapDisplay::hideBrushSizeHUD()
{
    if (m_brushSizeHUD && m_brushSizeHUD->isVisible()) {
        // Use smooth fade-out animation
        AnimationHelper::fadeOut(m_brushSizeHUD, AnimationHelper::FADE_DURATION);
    }
}
