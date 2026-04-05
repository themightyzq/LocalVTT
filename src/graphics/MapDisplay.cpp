#include "graphics/MapDisplay.h"
#include "graphics/SceneBuilder.h"
#include "ui/MainWindow.h"
#include "utils/AnimationHelper.h"
#include <QMessageBox>
#include <QApplication>
#include "graphics/GridOverlay.h"
#include "graphics/FogOfWar.h"
#include "graphics/PingIndicator.h"
#include "graphics/GMBeacon.h"
#include "graphics/LightingOverlay.h"
#include "graphics/WeatherEffect.h"
#include "graphics/FogMistEffect.h"
#include "graphics/LightningEffect.h"
#include "graphics/PointLightSystem.h"
#include "graphics/PointLight.h"
#include "graphics/SceneAnimationDriver.h"
#include "graphics/ZLayers.h"
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
#include <QPointer>
#include <QTimer>
#include <QPainter>
#include <QGraphicsTextItem>
#include <QDateTime>
#include <QApplication>
#include <QThread>
#include <QEventLoop>
#include <cmath>
#include <QtMath>

// Define static flag for app readiness
bool MapDisplay::s_appReadyForProgress = false;

MapDisplay::MapDisplay(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_mapItem(nullptr)
    , m_gridOverlay(nullptr)
    , m_fogOverlay(nullptr)
    , m_gridEnabled(true)
    , m_fogEnabled(true)
    , m_ownScene(true)
    , m_vttGridSize(0)
    , m_fogBrushSize(200)  // Default brush size (50% of max 400)
    , m_fogHideModeEnabled(false)
    , m_fogRectangleModeEnabled(false)
    , m_beaconColor(255, 70, 70)  // Red default (brighter)
    , m_isSelectingRectangle(false)
    , m_selectionRectIndicator(nullptr)
    , m_isPanning(false)
    , m_spacebarHeld(false)
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
    , m_loadingProgressWidget(nullptr)
    , m_imageLoader(nullptr)
    , m_zoomControlsEnabled(true)
    , m_lightingOverlay(nullptr)
    , m_animationDriver(nullptr)
    , m_pointLightPlacementMode(false)
    , m_currentLightPreset(LightPreset::Torch)
    , m_isDraggingLight(false)
    , m_selectedPointLightIndicator(nullptr)
    , m_mainWindow(nullptr)
    , m_fogBrushPreview(nullptr)
    , m_brushSizeHUD(nullptr)
    , m_hudFadeTimer(nullptr)
    , m_currentTool(ToolType::Pointer)
    , m_currentFogMode(FogToolMode::UnifiedFog)
    , m_isDraggingTool(false)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    // Create unified animation driver for atmosphere effects
    m_animationDriver = new SceneAnimationDriver(this);
    m_animationDriver->setScene(m_scene);
    m_animationDriver->start();

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    setDragMode(QGraphicsView::NoDrag);

    // Don't accept drops here - let MainWindow handle file drops
    setAcceptDrops(false);
    viewport()->setAcceptDrops(false);

    // CRITICAL FIX: Use black background for letterboxing support
    // When map doesn't match screen aspect ratio, black bars look professional
    setBackgroundBrush(QBrush(QColor(0, 0, 0)));

    setFocusPolicy(Qt::ClickFocus);

    // Disable context menu - right-click is used for panning
    setContextMenuPolicy(Qt::NoContextMenu);

    // Overlays are created by SceneBuilder on first map load
    m_lightingOverlay = nullptr;
    m_weatherEffect = nullptr;
    m_fogMistEffect = nullptr;
    m_lightningEffect = nullptr;
    m_pointLightSystem = nullptr;

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
    m_fogBrushPreview->setZValue(ZLayer::BrushPreview);

    m_scene->addItem(m_fogBrushPreview);

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
        m_lightingOverlay = nullptr;
        m_weatherEffect = nullptr;
        m_fogMistEffect = nullptr;
        m_lightningEffect = nullptr;
        m_pointLightSystem = nullptr;
        m_mapItem = nullptr;
        m_fogBrushPreview = nullptr;
        m_selectionRectIndicator = nullptr;
        m_selectedPointLightIndicator = nullptr;
        m_lightDebugItems.clear();
    }

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
        // Point lights persist across map loads (managed via getPointLightSystem())
    }

    // Build scene with all overlays
    SceneConfig config;
    config.gridEnabled = m_gridEnabled;
    config.fogEnabled = m_fogEnabled;
    config.vttGridSize = m_vttGridSize;
    config.fogState = fogState;
    config.fogChangeCallback = [this](const QRectF& dirtyRegion) {
        notifyFogChanged(dirtyRegion);
    };

    SceneContents contents = SceneBuilder::buildScene(m_scene, m_currentMap, config);
    if (!contents.mapItem) {
        return false;
    }

    applySceneContents(contents);

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
        // Point lights persist across map loads (managed via getPointLightSystem())
    }

    // Build scene with all overlays
    SceneConfig config;
    config.gridEnabled = m_gridEnabled;
    config.fogEnabled = m_fogEnabled;
    config.vttGridSize = m_vttGridSize;
    config.fogState = fogState;
    config.fogChangeCallback = [this](const QRectF& dirtyRegion) {
        notifyFogChanged(dirtyRegion);
    };

    SceneContents contents = SceneBuilder::buildScene(m_scene, m_currentMap, config);
    if (!contents.mapItem) {
        m_loadingProgressWidget->hideProgress();
        return false;
    }

    applySceneContents(contents);

    // Apply VTT lighting and parsed lights if available
    // CRITICAL FIX: Use already loaded VTT data instead of loading again
    if (isVTTFile && vttData.isValid) {
        m_loadingProgressWidget->setProgress(75);
        m_loadingProgressWidget->setLoadingText("Applying VTT features...");

        applyVTTLighting(vttData.globalLight, vttData.darkness);
        setParsedLights(vttData.lights);
    }

    // CRITICAL FIX: Ensure progress reaches 100% before hiding
    m_loadingProgressWidget->setProgress(100);
    m_loadingProgressWidget->setLoadingText("Loading complete");

    // Hide progress widget after a brief moment to show completion
    QPointer<MapDisplay> self = this;
    QTimer::singleShot(200, this, [self]() {
        if (self && self->m_loadingProgressWidget) self->m_loadingProgressWidget->hideProgress();
    });

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
        // Point lights persist across map loads (managed via getPointLightSystem())
    }

    m_loadingProgressWidget->setProgress(50);

    // Build scene with all overlays
    SceneConfig config;
    config.gridEnabled = m_gridEnabled;
    config.fogEnabled = m_fogEnabled;
    config.vttGridSize = m_vttGridSize;
    config.fogState = fogState;
    config.fogChangeCallback = [this](const QRectF& dirtyRegion) {
        notifyFogChanged(dirtyRegion);
    };

    SceneContents contents = SceneBuilder::buildScene(m_scene, m_currentMap, config);
    if (!contents.mapItem) {
        m_loadingProgressWidget->hideProgress();
        return false;
    }

    applySceneContents(contents);

    m_loadingProgressWidget->setProgress(75);

    // Process VTT data if available
    if (vttData.isValid) {
        // Apply VTT lighting data
        setParsedLights(vttData.lights);
    }

    m_loadingProgressWidget->setProgress(90);

    // Update shared displays
    updateSharedScene();

    m_loadingProgressWidget->setProgress(100);
    m_loadingProgressWidget->hideProgress();

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

    // VTT file lights stored for debug visualization only
    // Auto-conversion to PointLightSystem disabled (use manual placement instead)
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
        QGraphicsEllipseItem* ellipse = m_scene->addEllipse(rect, QPen(QColor(255, 255, 0, 200), 2), QBrush(Qt::NoBrush));
        ellipse->setZValue(ZLayer::BrushPreview);
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
            m_fogBrushPreview->setZValue(ZLayer::BrushPreview);
            m_fogBrushPreview->update();
        }
    }
}

void MapDisplay::shareScene(MapDisplay* sourceDisplay)
{
    if (!sourceDisplay) {
        return;
    }

    if (!sourceDisplay->getScene()) {
        DebugConsole::warning("Source scene is NULL, cannot share", "Graphics");
        return;
    }

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

    m_ownScene = false;
    m_scene = sourceDisplay->getScene();

    if (m_scene) {
        setScene(m_scene);
    }

    if (sourceDisplay->m_mapItem) {
        m_mapItem = sourceDisplay->m_mapItem;
    }

    if (!sourceDisplay->m_currentMap.isNull()) {
        m_currentMap = sourceDisplay->m_currentMap;
    }

    if (sourceDisplay->m_gridOverlay) {
        m_gridOverlay = sourceDisplay->m_gridOverlay;
    }
    if (sourceDisplay->m_fogOverlay) {
        m_fogOverlay = sourceDisplay->m_fogOverlay;
    }
    if (sourceDisplay->m_lightingOverlay) {
        m_lightingOverlay = sourceDisplay->m_lightingOverlay;
    }
    m_gridEnabled = sourceDisplay->m_gridEnabled;
    m_fogEnabled = sourceDisplay->m_fogEnabled;

    m_zoomFactor = sourceDisplay->m_zoomFactor;
    m_targetZoomFactor = m_zoomFactor;

    // Apply the initial transform to match the source display
    resetTransform();
    scale(m_zoomFactor, m_zoomFactor);

    if (m_mapItem) {
        m_mapItem->setVisible(true);
        m_mapItem->update();
    }

    if (m_scene) {
        m_scene->update();
    }

    update();
}

void MapDisplay::updateSharedScene()
{
    if (!m_ownScene && m_scene) {


        m_scene->invalidate();
        m_scene->update(m_scene->sceneRect());

        setScene(nullptr);
        setScene(m_scene);

        if (viewport()) {
            viewport()->update();
        }

        update();
    }
}

void MapDisplay::copyMapFrom(MapDisplay* sourceDisplay)
{
    if (!sourceDisplay) {
        return;
    }

    QImage sourceImage = sourceDisplay->getCurrentMapImage();
    if (sourceImage.isNull()) {
        return;
    }



    m_currentMap = sourceImage;

    if (m_mapItem) {
        m_mapItem->setPixmap(QPixmap::fromImage(m_currentMap));
    } else {
        m_mapItem = new QGraphicsPixmapItem(QPixmap::fromImage(m_currentMap));
        m_mapItem->setTransformationMode(Qt::SmoothTransformation);
        m_mapItem->setZValue(ZLayer::Map);

        if (m_scene) {
            m_scene->addItem(m_mapItem);
        }
    }

    if (m_scene && m_mapItem) {
        QRectF imageRect = m_mapItem->boundingRect();
        m_scene->setSceneRect(imageRect);
    }

    m_zoomFactor = sourceDisplay->m_zoomFactor;
    m_targetZoomFactor = m_zoomFactor;
    resetTransform();
    scale(m_zoomFactor, m_zoomFactor);

    if (m_scene) {
        m_scene->update();
    }
    update();
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
            m_scene->removeItem(m_selectionRectIndicator);
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

void MapDisplay::applySceneContents(const SceneContents& contents)
{
    m_mapItem = contents.mapItem;
    m_gridOverlay = contents.gridOverlay;
    m_fogOverlay = contents.fogOverlay;
    m_fogBrushPreview = contents.fogBrushPreview;

    // Effect overlays use lazy loading (created on demand)
    m_lightingOverlay = nullptr;
    m_weatherEffect = nullptr;
    m_fogMistEffect = nullptr;
    m_lightningEffect = nullptr;
    m_pointLightSystem = nullptr;
    m_selectionRectIndicator = nullptr;
    m_selectedPointLightIndicator = nullptr;
    m_lightDebugItems.clear();

    updateGrid();
    updateFog();
    notifyFogChanged();

    QPointer<MapDisplay> zoomSelf = this;
    QTimer::singleShot(10, this, [zoomSelf]() {
        if (zoomSelf) zoomSelf->setInitialZoom();
    });

    emit scenePopulated();
}

void MapDisplay::updateGrid()
{
    if (!m_gridOverlay || !m_scene) {
        return;
    }

    if (m_gridEnabled) {
        if (m_gridOverlay->scene() != m_scene) {
            m_scene->addItem(m_gridOverlay);
            m_gridOverlay->setZValue(ZLayer::Grid);
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

    if (m_fogEnabled) {
        if (m_fogOverlay->scene() != m_scene) {
            m_scene->addItem(m_fogOverlay);
            m_fogOverlay->setZValue(ZLayer::Fog);
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

}

void MapDisplay::wheelEvent(QWheelEvent *event)
{
    if (!m_zoomControlsEnabled) {
        event->ignore();
        return;
    }

    // Check for modifier keys to determine behavior:
    // - Ctrl/Cmd + scroll = zoom (standard Mac/Windows behavior)
    // - Plain scroll = pan vertically (trackpad-friendly)
    // - Shift + scroll = pan horizontally
    bool hasControlModifier = event->modifiers() & Qt::ControlModifier;
    bool hasShiftModifier = event->modifiers() & Qt::ShiftModifier;

    // Without Ctrl/Cmd, use scroll for panning instead of zooming
    if (!hasControlModifier) {
        // Pan mode - scroll wheel moves the view
        int deltaY = event->angleDelta().y();
        int deltaX = event->angleDelta().x();

        // Shift+scroll = horizontal pan
        if (hasShiftModifier) {
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - deltaY);
        } else {
            // Normal scroll = vertical pan, also respect horizontal component
            verticalScrollBar()->setValue(verticalScrollBar()->value() - deltaY);
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - deltaX);
        }
        event->accept();
        return;
    }

    // Ctrl/Cmd + scroll = zoom
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
    // Panning options:
    // - Middle mouse button (always)
    // - Spacebar + Left click (trackpad-friendly)
    // - Shift + Right click (avoids trackpad zoom interference)
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && m_spacebarHeld) ||
        (event->button() == Qt::RightButton && (event->modifiers() & Qt::ShiftModifier))) {
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
    } else if (event->button() == Qt::LeftButton && m_pointLightPlacementMode) {
        // Handle point light placement/selection/dragging
        QPointF scenePos = mapToScene(event->pos());

        // First, check if we clicked on an existing light
        PointLightSystem* lightSystem = getPointLightSystem();
        QUuid clickedLightId = lightSystem->lightAtPosition(scenePos);

        if (!clickedLightId.isNull()) {
            // Clicked on an existing light - select it and prepare for dragging
            selectLight(clickedLightId);
            m_isDraggingLight = true;
            // Calculate offset from light center for smooth dragging
            const PointLight* light = lightSystem->getLight(clickedLightId);
            if (light) {
                m_lightDragOffset = light->position - scenePos;
            }
            setCursor(Qt::ClosedHandCursor);
        } else {
            // Clicked on empty space - place a new light
            deselectLight();
            QUuid newLightId = lightSystem->addLightAtPosition(scenePos, m_currentLightPreset);
            selectLight(newLightId);
            qDebug() << "MapDisplay: Added light at" << scenePos << "with preset" << static_cast<int>(m_currentLightPreset);
        }
        event->accept();
        return;
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
    } else if (m_isDraggingLight && !m_selectedPointLightId.isNull()) {
        // Drag the selected light to new position
        QPointF scenePos = mapToScene(event->pos());
        QPointF newPos = scenePos + m_lightDragOffset;

        PointLightSystem* lightSystem = getPointLightSystem();
        PointLight* light = lightSystem->getLight(m_selectedPointLightId);
        if (light) {
            light->position = newPos;
            lightSystem->updateLight(m_selectedPointLightId, *light);
            updateSelectionIndicator();
        }
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
    // Release panning for any button that started it
    if ((event->button() == Qt::MiddleButton && m_isPanning) ||
        (event->button() == Qt::RightButton && m_isPanning) ||
        (event->button() == Qt::LeftButton && m_isPanning)) {
        m_isPanning = false;
        updateToolCursor();

        // Calculate release velocity from recent movement history
        calculateReleaseVelocity();

        // Start momentum animation if velocity is significant
        if (m_panVelocity.manhattanLength() > 2.0) {
            m_smoothPanTimer->start();
        }
    } else if (event->button() == Qt::LeftButton && m_isDraggingLight) {
        // End light dragging
        m_isDraggingLight = false;
        if (m_pointLightPlacementMode) {
            setCursor(Qt::CrossCursor);
        } else {
            updateToolCursor();
        }
        event->accept();
        return;
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
    // CLAUDE.md 3.6: NO number keys for tool switching
    // Number key handlers removed per CLAUDE.md compliance

    // Spacebar for pan mode (like Photoshop)
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacebarHeld = true;
        setCursor(Qt::OpenHandCursor);
        return;
    }

    // Delete/Backspace to remove selected point light
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) &&
        !m_selectedPointLightId.isNull()) {
        PointLightSystem* lightSystem = getPointLightSystem();
        QUuid lightToRemove = m_selectedPointLightId;
        deselectLight();
        lightSystem->removeLight(lightToRemove);
        qDebug() << "MapDisplay: Deleted light" << lightToRemove;
        return;
    }

    // Escape to deselect light or cancel light placement mode
    if (event->key() == Qt::Key_Escape) {
        if (!m_selectedPointLightId.isNull()) {
            deselectLight();
            return;
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

            // Key_0 removed - use '/' for fit-to-screen (CLAUDE.md 3.6)

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

            // Key_4, Key_5, Key_6 removed - obscure zoom presets (CLAUDE.md UI consistency)

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
            int newSize = qMin(400, m_fogBrushSize + 10);
            setFogBrushSize(newSize);
            showBrushSizeHUD(newSize);
            return;
        }
    }


    // Token deletion
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Handle deletion of selected items if needed
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void MapDisplay::keyReleaseEvent(QKeyEvent *event)
{
    // Release spacebar pan mode
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacebarHeld = false;
        if (!m_isPanning) {
            updateToolCursor();  // Restore normal cursor
        }
        return;
    }

    QGraphicsView::keyReleaseEvent(event);
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

    PingIndicator* ping = new PingIndicator(scenePos);
    m_scene->addItem(ping);
    ping->startAnimation();
}

void MapDisplay::createGMBeacon(const QPointF& scenePos)
{
    if (!m_scene) {
        return;
    }

    // Calculate beacon size based on MAP size (not viewport) for consistent scaling
    // Use the smaller dimension of the map to ensure beacon is visible
    QRectF sceneRect = m_scene->sceneRect();
    qreal mapDimension = qMin(sceneRect.width(), sceneRect.height());
    // Fallback to viewport if no map loaded
    if (mapDimension <= 0) {
        mapDimension = viewport()->width();
    }

    GMBeacon* beacon = new GMBeacon(scenePos, mapDimension);
    beacon->setColor(m_beaconColor);  // Apply selected color BEFORE animation starts
    m_scene->addItem(beacon);
    beacon->startAnimation();  // Start animation AFTER color is set
}

void MapDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());

        // In light placement mode, double-click on existing light opens edit dialog
        if (m_pointLightPlacementMode) {
            PointLightSystem* lightSystem = getPointLightSystem();
            if (!lightSystem) return;  // Defensive null-check
            QUuid clickedLightId = lightSystem->lightAtPosition(scenePos);
            if (!clickedLightId.isNull()) {
                // Emit signal to open edit dialog for this light
                emit pointLightDoubleClicked(clickedLightId);
                event->accept();
                return;
            }
        }

        // Default: create GM beacon on double-click (global behavior)
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
        m_lightingOverlay = new LightingOverlay();
        m_scene->addItem(m_lightingOverlay);
        m_lightingOverlay->setZValue(ZLayer::LightingOverlay);
        // Keep the default enabled state from LightingOverlay constructor
        // which is true - this ensures menu state matches actual state
    }
    return m_lightingOverlay;
}

// Weather Effects implementation with lazy loading
WeatherEffect* MapDisplay::getWeatherEffect()
{
    if (!m_weatherEffect) {
        m_weatherEffect = new WeatherEffect();
        m_scene->addItem(m_weatherEffect);
        // Z-value already set in WeatherEffect constructor (50.0)
        // Set scene bounds based on current map
        if (m_mapItem) {
            m_weatherEffect->setSceneBounds(m_mapItem->boundingRect());
        }
        // Wire to unified animation driver
        if (m_animationDriver) {
            connect(m_animationDriver, &SceneAnimationDriver::tick,
                    m_weatherEffect, &WeatherEffect::advanceAnimation);
        }
    }
    return m_weatherEffect;
}

// Fog/Mist Effect implementation with lazy loading
FogMistEffect* MapDisplay::getFogMistEffect()
{
    if (!m_fogMistEffect) {
        m_fogMistEffect = new FogMistEffect();
        m_scene->addItem(m_fogMistEffect);
        // Z-value already set in FogMistEffect constructor (40.0)
        // Set scene bounds based on current map
        if (m_mapItem) {
            m_fogMistEffect->setSceneBounds(m_mapItem->boundingRect());
        }
        // Wire to unified animation driver
        if (m_animationDriver) {
            connect(m_animationDriver, &SceneAnimationDriver::tick,
                    m_fogMistEffect, &FogMistEffect::advanceAnimation);
        }
    }
    return m_fogMistEffect;
}

// Lightning Effect implementation with lazy loading
LightningEffect* MapDisplay::getLightningEffect()
{
    if (!m_lightningEffect) {
        m_lightningEffect = new LightningEffect();
        m_scene->addItem(m_lightningEffect);
        // Z-value already set in LightningEffect constructor (70.0)
        // Set scene bounds based on current map
        if (m_mapItem) {
            m_lightningEffect->setSceneBounds(m_mapItem->boundingRect());
        }
        // Wire to unified animation driver
        if (m_animationDriver) {
            connect(m_animationDriver, &SceneAnimationDriver::tick,
                    m_lightningEffect, &LightningEffect::advanceAnimation);
        }
    }
    return m_lightningEffect;
}

// Point Light System implementation with lazy loading
PointLightSystem* MapDisplay::getPointLightSystem()
{
    if (!m_pointLightSystem) {
        m_pointLightSystem = new PointLightSystem();
        m_scene->addItem(m_pointLightSystem);
        // Z-value already set in PointLightSystem constructor (35.0)
        // Set scene bounds based on current map
        if (m_mapItem) {
            m_pointLightSystem->setSceneBounds(m_mapItem->boundingRect());
        }
        // Wire to unified animation driver
        if (m_animationDriver) {
            connect(m_animationDriver, &SceneAnimationDriver::tick,
                    m_pointLightSystem, &PointLightSystem::advanceAnimation);
        }
    }
    return m_pointLightSystem;
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

// Point light management
void MapDisplay::setPointLightPlacementMode(bool enabled)
{
    m_pointLightPlacementMode = enabled;
    if (enabled) {
        setCursor(Qt::CrossCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void MapDisplay::addPointLight(const QPointF& position)
{
    // Add a torch light at the given position by default
    getPointLightSystem()->addLightAtPosition(position, LightPreset::Torch);
}

void MapDisplay::removePointLight(const QUuid& lightId)
{
    getPointLightSystem()->removeLight(lightId);
}

void MapDisplay::clearAllPointLights()
{
    getPointLightSystem()->removeAllLights();
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


// Weather control stubs (actual weather via lazy-loaded getWeatherEffect())
void MapDisplay::setWeatherType(int weatherType)
{
    Q_UNUSED(weatherType);
    // Stub - use getWeatherEffect() for actual weather control
}

int MapDisplay::getWeatherType() const
{
    return 0; // Default - see getWeatherEffect() for actual control
}

void MapDisplay::setWeatherIntensity(float intensity)
{
    Q_UNUSED(intensity);
    // Stub - use getWeatherEffect() for actual weather control
}

float MapDisplay::getWeatherIntensity() const
{
    return 0.5f; // Default - see getWeatherEffect() for actual control
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

// ============================================================================
// Point Light Selection and Manipulation
// ============================================================================

void MapDisplay::selectLight(const QUuid& lightId)
{
    if (lightId == m_selectedPointLightId) {
        return; // Already selected
    }

    m_selectedPointLightId = lightId;
    updateSelectionIndicator();

    qDebug() << "MapDisplay: Selected light" << lightId;
    emit pointLightDoubleClicked(lightId); // Reuse signal for property editing
}

void MapDisplay::deselectLight()
{
    if (m_selectedPointLightId.isNull()) {
        return; // Nothing to deselect
    }

    m_selectedPointLightId = QUuid();

    // Remove selection indicator
    if (m_selectedPointLightIndicator) {

        m_scene->removeItem(m_selectedPointLightIndicator);
        delete m_selectedPointLightIndicator;
        m_selectedPointLightIndicator = nullptr;
    }

    qDebug() << "MapDisplay: Deselected light";
}

void MapDisplay::updateSelectionIndicator()
{
    // Remove existing indicator
    if (m_selectedPointLightIndicator) {

        m_scene->removeItem(m_selectedPointLightIndicator);
        delete m_selectedPointLightIndicator;
        m_selectedPointLightIndicator = nullptr;
    }

    // Create new indicator if we have a selected light
    if (!m_selectedPointLightId.isNull()) {
        PointLightSystem* lightSystem = getPointLightSystem();
        const PointLight* light = lightSystem->getLight(m_selectedPointLightId);

        if (light) {


            // Create a ring indicator around the light
            qreal radius = light->radius * 0.3; // Selection ring at 30% of light radius
            if (radius < 20) radius = 20; // Minimum visible size

            m_selectedPointLightIndicator = new QGraphicsEllipseItem(
                light->position.x() - radius,
                light->position.y() - radius,
                radius * 2,
                radius * 2
            );

            // Style: cyan dashed ring
            QPen pen(QColor(0, 200, 255), 3);
            pen.setStyle(Qt::DashLine);
            m_selectedPointLightIndicator->setPen(pen);
            m_selectedPointLightIndicator->setBrush(Qt::NoBrush);
            m_selectedPointLightIndicator->setZValue(ZLayer::ToolPreview);

            m_scene->addItem(m_selectedPointLightIndicator);
        }
    }
}

void MapDisplay::setCurrentLightPreset(LightPreset preset)
{
    m_currentLightPreset = preset;
    qDebug() << "MapDisplay: Current light preset set to" << static_cast<int>(preset);
}
