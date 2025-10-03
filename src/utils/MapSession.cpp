#include "utils/MapSession.h"
#include "graphics/MapDisplay.h"
#include "graphics/GridOverlay.h"
#include "graphics/LightingOverlay.h"
#include "utils/VTTLoader.h"
#include "utils/SettingsManager.h"
#include "utils/DebugConsole.h"
#include "utils/MemoryManager.h"
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QGraphicsScene>
#include <QPainter>
#include <QByteArray>

MapSession::MapSession(const QString& filePath)
    : m_filePath(filePath)
    , m_gridEnabled(true)
    , m_fogEnabled(false)
    , m_zoomLevel(1.0)
    , m_viewCenter(0, 0)
    , m_isActive(false)
    , m_sceneCached(false)
{
    QFileInfo fileInfo(filePath);
    m_fileName = fileInfo.baseName();
    initializeFogPath();
}

MapSession::~MapSession()
{
    // Report memory release if we have cached images
    if (!m_cachedImage.isNull()) {
        MemoryManager::instance().reportImageReleased(m_cachedImage);
    }

    // Clear cached scene reference
    m_cachedScene.reset();
}

bool MapSession::loadImage()
{
    QFileInfo fileInfo(m_filePath);
    if (!fileInfo.exists() || !fileInfo.isReadable() || fileInfo.size() == 0) {
        return false;
    }

    // Check if we need to reload (file modified or no cache or memory was released)
    QDateTime currentModified = fileInfo.lastModified();
    if (!m_cachedImage.isNull() && m_fileLastModified == currentModified && !m_memoryReleased) {
        // Cache is valid, no need to reload
        return true;
    }

    // Cache is invalid or doesn't exist, load from file
    DebugConsole::info(QString("Loading image from file (cache miss): %1").arg(m_filePath), "Session");

    if (VTTLoader::isVTTFile(m_filePath)) {
        // Load VTT file
        m_cachedVTTData = VTTLoader::loadVTT(m_filePath);
        if (m_cachedVTTData.mapImage.isNull()) {
            DebugConsole::error(QString("Failed to load VTT file: %1").arg(m_cachedVTTData.errorMessage), "Session");
            return false;
        }
        m_cachedImage = m_cachedVTTData.mapImage;
    } else {
        // Load regular image
        m_cachedImage = QImage(m_filePath);
        if (m_cachedImage.isNull()) {
            DebugConsole::error(QString("Failed to load image: %1").arg(m_filePath), "Session");
            return false;
        }
        // Reset VTT data for non-VTT files
        m_cachedVTTData = VTTLoader::VTTData();
    }

    m_fileLastModified = currentModified;
    m_memoryReleased = false;  // Mark that we have the image in memory

    // Report memory usage to the manager
    MemoryManager::instance().reportImageLoaded(m_cachedImage);

    DebugConsole::info("Image loaded and cached successfully", "Session");
    return true;
}



void MapSession::activateSession(MapDisplay* mapDisplay)
{
    DebugConsole::info(QString("Activating session for file: %1").arg(m_filePath), "Session");

    if (!mapDisplay) {
        DebugConsole::error("mapDisplay is null!", "Session");
        return;
    }

    // Ensure image is loaded and cached
    if (!loadImage()) {
        DebugConsole::error("Failed to load/cache image!", "Session");
        return;
    }

    // TODO: Scene caching disabled for stability - just use image caching for now
    // This still provides significant performance improvement over full file reload

    // Cache miss or scene cache invalid, but we have image cache - use fast loading
    DebugConsole::performance("Loading from image cache (medium speed path)", "Session");
    bool success = mapDisplay->loadImageFromCache(m_cachedImage, m_cachedVTTData);
    if (!success) {
        DebugConsole::error("Failed to load from cache!", "Session");
        return;
    }

    m_isActive = true;

    mapDisplay->setGridEnabled(m_gridEnabled);
    mapDisplay->setFogEnabled(m_fogEnabled);

    // Load and apply per-map grid calibration if available
    if (hasGridCalibration()) {
        double tvSize, viewingDistance;
        int gridSize;
        QPointF gridOffset;
        loadGridCalibration(tvSize, viewingDistance, gridSize, gridOffset);

        // Apply grid size to the display
        if (mapDisplay->getGridOverlay()) {
            mapDisplay->getGridOverlay()->setGridSize(gridSize);
            DebugConsole::info(QString("Applied per-map grid calibration - size: %1").arg(gridSize), "Session");
        }
    }

    // Only restore zoom/view for previously opened maps (when we have a saved zoom level)
    // For new maps, let the fitMapToView in loadImage take effect
    bool isFirstLoad = (m_zoomLevel == 1.0 && m_viewCenter == QPointF(0, 0));
    if (!isFirstLoad) {
        mapDisplay->syncZoomLevel(m_zoomLevel, m_viewCenter);
    }

    loadFogState(mapDisplay);
    // Manual lights removed - only basic ambient lighting now

    // TODO: Scene caching disabled for stability - image caching provides the main benefit
}

void MapSession::deactivateSession(MapDisplay* mapDisplay)
{
    if (!mapDisplay) {
        return;
    }

    m_isActive = false;

    // Save state for restoration (scene caching disabled for stability)
    saveFogState(mapDisplay);
    // Manual lights removed - only basic ambient lighting now

    // Memory optimization: Release image memory for inactive tabs
    // We keep the metadata but free the actual image data
    // The image will be reloaded from disk when the tab is reactivated
    // Only release if under memory pressure or if configured to do so
    if (MemoryManager::instance().shouldReleaseInactiveTabs()) {
        releaseImageMemory();
        DebugConsole::performance(QString("Released image memory for inactive tab: %1").arg(m_fileName), "Memory");
    }
}

void MapSession::saveFogState(MapDisplay* mapDisplay)
{
    if (!mapDisplay) {
        return;
    }

    // Memory optimization: Compress fog state for inactive tabs
    QByteArray uncompressed = mapDisplay->saveFogState();
    if (!uncompressed.isEmpty()) {
        m_savedFogState = qCompress(uncompressed, 9);  // Max compression
        DebugConsole::performance(QString("Compressed fog state from %1 to %2 bytes")
            .arg(uncompressed.size()).arg(m_savedFogState.size()), "Memory");
    } else {
        m_savedFogState.clear();
    }
}

void MapSession::loadFogState(MapDisplay* mapDisplay)
{
    if (!mapDisplay || m_savedFogState.isEmpty()) {
        return;
    }

    // Decompress fog state if it was compressed
    QByteArray fogData = m_savedFogState;
    if (fogData.size() >= 4) {
        // Check if data is compressed (Qt compression adds a 4-byte header)
        QByteArray header = fogData.left(4);
        if (header[0] == 'q' && header[1] == 'c' && header[2] == 'd' && header[3] == 'a') {
            // Data is compressed, decompress it
            fogData = qUncompress(m_savedFogState);
            if (fogData.isEmpty()) {
                DebugConsole::error("Failed to decompress fog state", "Memory");
                return;
            }
        }
    }

    mapDisplay->loadFogState(fogData);
}

void MapSession::initializeFogPath()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(m_filePath.toUtf8());
    QString hashString = hash.result().toHex();

    m_fogFilePath = dataDir + QDir::separator() + hashString + "_fog.dat";
}


void MapSession::cacheCurrentScene(MapDisplay* mapDisplay)
{
    if (!mapDisplay) {
        return;
    }

    QGraphicsScene* scene = mapDisplay->scene();
    if (!scene) {
        m_sceneCached = false;
        return;
    }

    try {
        // Create a shared pointer to the current scene for caching
        // Note: We need to be careful here - we're sharing ownership of the scene
        // The scene should not be deleted while cached
        m_cachedScene = std::shared_ptr<QGraphicsScene>(scene, [](QGraphicsScene*) {
            // Custom deleter that does nothing - the scene is managed elsewhere
            // This prevents double deletion
        });

        m_sceneCached = true;
        DebugConsole::performance(QString("Scene cached successfully for %1").arg(m_fileName), "Session");
    } catch (const std::exception& e) {
        DebugConsole::error(QString("Failed to cache scene: %1").arg(e.what()), "Session");
        m_sceneCached = false;
    }
}

bool MapSession::restoreCachedScene(MapDisplay* mapDisplay)
{
    if (!mapDisplay || !m_sceneCached) {
        return false;
    }

    // Check if we have a cached scene to restore
    if (!m_cachedScene || m_cachedImage.isNull()) {
        DebugConsole::warning("Scene cache invalid - no cached scene or image", "Session");
        return false;
    }

    try {
        // Restore the scene directly
        mapDisplay->setScene(m_cachedScene.get());

        // Update the current map reference
        mapDisplay->setCachedImage(m_cachedImage);

        DebugConsole::performance("Scene successfully restored from cache", "Session");
        return true;
    } catch (const std::exception& e) {
        DebugConsole::error(QString("Failed to restore cached scene: %1").arg(e.what()), "Session");
        m_sceneCached = false;
        return false;
    }
}

// Grid calibration persistence methods
void MapSession::saveGridCalibration(double tvSize, double viewingDistance, int gridSize, const QPointF& gridOffset)
{
    SettingsManager& settings = SettingsManager::instance();
    settings.saveMapGridCalibration(m_filePath, tvSize, viewingDistance, gridSize, gridOffset);
}

void MapSession::loadGridCalibration(double& tvSize, double& viewingDistance, int& gridSize, QPointF& gridOffset)
{
    SettingsManager& settings = SettingsManager::instance();
    settings.loadMapGridCalibration(m_filePath, tvSize, viewingDistance, gridSize, gridOffset);
}

bool MapSession::hasGridCalibration() const
{
    SettingsManager& settings = SettingsManager::instance();
    return settings.hasMapGridCalibration(m_filePath);
}
