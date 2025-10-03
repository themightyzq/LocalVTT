#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QSettings>
#include <QRect>
#include <QPointF>
#include <QColor>
#include <QGlobalStatic>

class SettingsManager
{
public:
    static SettingsManager& instance();
    
    void saveWindowGeometry(const QString& windowName, const QRect& geometry);
    QRect loadWindowGeometry(const QString& windowName, const QRect& defaultGeometry);
    
    void saveGridEnabled(bool enabled);
    bool loadGridEnabled();
    
    void saveFogEnabled(bool enabled);
    bool loadFogEnabled();
    
    void saveZoomLevel(qreal zoomLevel);
    qreal loadZoomLevel();
    
    void saveLastDirectory(const QString& directory);
    QString loadLastDirectory();
    
    void saveRecentFiles(const QStringList& files);
    QStringList loadRecentFiles();
    
    // Grid calibration settings (global defaults)
    void saveTvSize(double size);
    double loadTvSize();

    void saveViewingDistance(double distance);
    double loadViewingDistance();

    void saveGridSize(int size);
    int loadGridSize();

    // Per-map grid calibration settings
    void saveMapGridCalibration(const QString& mapPath, double tvSize, double viewingDistance, int gridSize, const QPointF& gridOffset);
    void loadMapGridCalibration(const QString& mapPath, double& tvSize, double& viewingDistance, int& gridSize, QPointF& gridOffset);
    bool hasMapGridCalibration(const QString& mapPath);
    void removeMapGridCalibration(const QString& mapPath);

    // GM Beacon settings
    void saveGMBeaconSize(int size);
    int loadGMBeaconSize();

    void saveGMBeaconColor(const QColor& color);
    QColor loadGMBeaconColor();

    void saveGMBeaconShape(int shape);
    int loadGMBeaconShape();

    void saveGMBeaconOpacity(int opacity);
    int loadGMBeaconOpacity();

    // Fog/Mist settings
    void saveFogAnimationSpeed(int speed);
    int loadFogAnimationSpeed();

    void saveFogOpacity(int opacity);
    int loadFogOpacity();

    void saveFogTextureIndex(int index);
    int loadFogTextureIndex();

    // Performance settings
    void saveAnimationQuality(int quality);
    int loadAnimationQuality();

    void saveSmoothAnimations(bool enabled);
    bool loadSmoothAnimations();

    void saveUpdateFrequency(int frequency);
    int loadUpdateFrequency();

    // Display settings
    void saveGridOpacity(int opacity);
    int loadGridOpacity();

    void saveGridColor(const QColor& color);
    QColor loadGridColor();

    void saveDefaultFogBrushSize(int size);
    int loadDefaultFogBrushSize();


    // Interaction settings
    void saveWheelZoomEnabled(bool enabled);
    bool loadWheelZoomEnabled();

    void clearAllSettings();

    // Force immediate sync to disk
    void syncSettings();

    // Q_GLOBAL_STATIC requires public constructor/destructor
    SettingsManager();
    ~SettingsManager() = default;

private:
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    // Helper for generating consistent map keys
    QString generateMapKey(const QString& mapPath);

    QSettings* m_settings;
};

#endif // SETTINGSMANAGER_H
