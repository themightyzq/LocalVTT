#ifndef MAPSESSION_H
#define MAPSESSION_H

#include <QString>
#include <QImage>
#include <QPointF>
#include <QByteArray>
#include <QList>
#include <QPixmap>
#include <QDateTime>
#include <memory>
#include "utils/VTTLoader.h"

class MapDisplay;
class QGraphicsScene;

class MapSession
{
public:
    explicit MapSession(const QString& filePath);
    ~MapSession();

    const QString& filePath() const { return m_filePath; }
    const QString& fileName() const { return m_fileName; }
    const QImage& image() const { return m_cachedImage; }  // Return cached image
    
    bool loadImage();
    void activateSession(MapDisplay* mapDisplay);
    void deactivateSession(MapDisplay* mapDisplay);
    
    void setGridEnabled(bool enabled) { m_gridEnabled = enabled; }
    bool isGridEnabled() const { return m_gridEnabled; }
    
    void setFogEnabled(bool enabled) { m_fogEnabled = enabled; }
    bool isFogEnabled() const { return m_fogEnabled; }
    
    void setZoomLevel(qreal zoom) { m_zoomLevel = zoom; }
    qreal zoomLevel() const { return m_zoomLevel; }
    
    void setViewCenter(const QPointF& center) { m_viewCenter = center; }
    const QPointF& viewCenter() const { return m_viewCenter; }

    // Grid calibration persistence per map
    void saveGridCalibration(double tvSize, double viewingDistance, int gridSize, const QPointF& gridOffset);
    void loadGridCalibration(double& tvSize, double& viewingDistance, int& gridSize, QPointF& gridOffset);
    bool hasGridCalibration() const;
    
    void saveFogState(MapDisplay* mapDisplay);
    void loadFogState(MapDisplay* mapDisplay);


    // Image and scene caching for performance
    bool hasImageCache() const { return !m_cachedImage.isNull(); }
    bool hasSceneCache() const { return m_sceneCached; }
    void invalidateCache() {
        m_sceneCached = false;
        m_cachedPixmap = QPixmap();
        m_cachedImage = QImage();
        m_cachedVTTData = VTTLoader::VTTData();
    }

    // Memory optimization: release image memory for inactive tabs
    void releaseImageMemory() {
        m_cachedImage = QImage();
        m_cachedPixmap = QPixmap();
        // Clear VTT data except essential grid info
        if (m_cachedVTTData.isValid) {
            int gridSize = m_cachedVTTData.pixelsPerGrid;
            m_cachedVTTData = VTTLoader::VTTData();
            m_cachedVTTData.pixelsPerGrid = gridSize;  // Keep grid size for restoration
            m_cachedVTTData.isValid = true;
        }
        m_memoryReleased = true;
    }

private:
    QString m_filePath;
    QString m_fileName;
    // QImage m_image;  // REMOVED - deprecated, was wasting memory

    // Cached data for fast tab switching
    QImage m_cachedImage;
    VTTLoader::VTTData m_cachedVTTData;
    QDateTime m_fileLastModified;
    bool m_memoryReleased = false;  // Track if memory was released for inactive tab

    bool m_gridEnabled;
    bool m_fogEnabled;
    qreal m_zoomLevel;
    QPointF m_viewCenter;

    bool m_isActive;
    QString m_fogFilePath;
    QByteArray m_savedFogState;


    // Scene caching for fast tab switching
    bool m_sceneCached;
    QPixmap m_cachedPixmap;
    std::shared_ptr<QGraphicsScene> m_cachedScene;

    void initializeFogPath();
    void cacheCurrentScene(MapDisplay* mapDisplay);
    bool restoreCachedScene(MapDisplay* mapDisplay);
};

#endif // MAPSESSION_H