#include "utils/SettingsManager.h"
#include "utils/DebugConsole.h"
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QGlobalStatic>

// Use Q_GLOBAL_STATIC for thread-safe, lazy initialization without recursion issues
Q_GLOBAL_STATIC(SettingsManager, g_settingsManagerInstance)

SettingsManager& SettingsManager::instance()
{
    return *g_settingsManagerInstance();
}

SettingsManager::SettingsManager()
{
    m_settings = new QSettings("LocalVTT", "LocalVTT");
}

void SettingsManager::saveWindowGeometry(const QString& windowName, const QRect& geometry)
{
    m_settings->beginGroup("WindowGeometry");
    m_settings->setValue(windowName, geometry);
    m_settings->endGroup();
    m_settings->sync();
}

QRect SettingsManager::loadWindowGeometry(const QString& windowName, const QRect& defaultGeometry)
{
    m_settings->beginGroup("WindowGeometry");
    QRect geometry = m_settings->value(windowName, defaultGeometry).toRect();
    m_settings->endGroup();
    return geometry;
}

void SettingsManager::saveGridEnabled(bool enabled)
{
    m_settings->setValue("gridEnabled", enabled);
    m_settings->sync();
}

bool SettingsManager::loadGridEnabled()
{
    return m_settings->value("gridEnabled", true).toBool();
}

void SettingsManager::saveFogEnabled(bool enabled)
{
    m_settings->setValue("fogEnabled", enabled);
    m_settings->sync();
}

bool SettingsManager::loadFogEnabled()
{
    return m_settings->value("fogEnabled", false).toBool();
}

void SettingsManager::saveZoomLevel(qreal zoomLevel)
{
    m_settings->setValue("zoomLevel", zoomLevel);
    m_settings->sync();
}

qreal SettingsManager::loadZoomLevel()
{
    return m_settings->value("zoomLevel", 1.0).toReal();
}

void SettingsManager::saveLastDirectory(const QString& directory)
{
    m_settings->setValue("lastMapDirectory", directory);
    m_settings->sync();
}

QString SettingsManager::loadLastDirectory()
{
    return m_settings->value("lastMapDirectory", 
                            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).toString();
}

void SettingsManager::saveRecentFiles(const QStringList& files)
{
    m_settings->setValue("recentFiles", files);
    m_settings->sync();
}

QStringList SettingsManager::loadRecentFiles()
{
    return m_settings->value("recentFiles").toStringList();
}

void SettingsManager::saveTvSize(double size)
{
    m_settings->setValue("gridCalibration/tvSize", size);
    m_settings->sync();
}

double SettingsManager::loadTvSize()
{
    return m_settings->value("gridCalibration/tvSize", 55.0).toDouble();
}

void SettingsManager::saveViewingDistance(double distance)
{
    m_settings->setValue("gridCalibration/viewingDistance", distance);
    m_settings->sync();
}

double SettingsManager::loadViewingDistance()
{
    return m_settings->value("gridCalibration/viewingDistance", 72.0).toDouble();
}

void SettingsManager::saveGridSize(int size)
{
    m_settings->setValue("gridCalibration/gridSize", size);
    m_settings->sync();
}

int SettingsManager::loadGridSize()
{
    return m_settings->value("gridCalibration/gridSize", 50).toInt();
}

// GM Beacon settings
void SettingsManager::saveGMBeaconSize(int size)
{
    m_settings->setValue("gmBeacon/size", size);
    m_settings->sync();
}

int SettingsManager::loadGMBeaconSize()
{
    return m_settings->value("gmBeacon/size", 20).toInt();
}

void SettingsManager::saveGMBeaconColor(const QColor& color)
{
    m_settings->setValue("gmBeacon/color", color);
    m_settings->sync();
}

QColor SettingsManager::loadGMBeaconColor()
{
    return m_settings->value("gmBeacon/color", QColor(74, 158, 255)).value<QColor>();
}

void SettingsManager::saveGMBeaconShape(int shape)
{
    m_settings->setValue("gmBeacon/shape", shape);
    m_settings->sync();
}

int SettingsManager::loadGMBeaconShape()
{
    return m_settings->value("gmBeacon/shape", 0).toInt();
}

void SettingsManager::saveGMBeaconOpacity(int opacity)
{
    m_settings->setValue("gmBeacon/opacity", opacity);
    m_settings->sync();
}

int SettingsManager::loadGMBeaconOpacity()
{
    return m_settings->value("gmBeacon/opacity", 90).toInt();
}

// Fog/Mist settings
void SettingsManager::saveFogAnimationSpeed(int speed)
{
    m_settings->setValue("fog/animationSpeed", speed);
    m_settings->sync();
}

int SettingsManager::loadFogAnimationSpeed()
{
    return m_settings->value("fog/animationSpeed", 50).toInt();
}

void SettingsManager::saveFogOpacity(int opacity)
{
    m_settings->setValue("fog/opacity", opacity);
    m_settings->sync();
}

int SettingsManager::loadFogOpacity()
{
    return m_settings->value("fog/opacity", 80).toInt();
}

void SettingsManager::saveFogTextureIndex(int index)
{
    m_settings->setValue("fog/textureIndex", index);
    m_settings->sync();
}

int SettingsManager::loadFogTextureIndex()
{
    return m_settings->value("fog/textureIndex", 0).toInt();
}

// Performance settings
void SettingsManager::saveAnimationQuality(int quality)
{
    m_settings->setValue("performance/animationQuality", quality);
    m_settings->sync();
}

int SettingsManager::loadAnimationQuality()
{
    return m_settings->value("performance/animationQuality", 1).toInt(); // medium
}

void SettingsManager::saveSmoothAnimations(bool enabled)
{
    m_settings->setValue("performance/smoothAnimations", enabled);
    m_settings->sync();
}

bool SettingsManager::loadSmoothAnimations()
{
    return m_settings->value("performance/smoothAnimations", true).toBool();
}

void SettingsManager::saveUpdateFrequency(int frequency)
{
    m_settings->setValue("performance/updateFrequency", frequency);
    m_settings->sync();
}

int SettingsManager::loadUpdateFrequency()
{
    return m_settings->value("performance/updateFrequency", 60).toInt();
}

// Display settings
void SettingsManager::saveGridOpacity(int opacity)
{
    m_settings->setValue("display/gridOpacity", opacity);
    m_settings->sync();
}

int SettingsManager::loadGridOpacity()
{
    return m_settings->value("display/gridOpacity", 50).toInt();
}

void SettingsManager::saveGridColor(const QColor& color)
{
    m_settings->setValue("display/gridColor", color);
    m_settings->sync();
}

QColor SettingsManager::loadGridColor()
{
    return m_settings->value("display/gridColor", QColor(255, 255, 255, 128)).value<QColor>();
}

void SettingsManager::saveDefaultFogBrushSize(int size)
{
    m_settings->setValue("display/defaultFogBrushSize", size);
    m_settings->sync();
}

int SettingsManager::loadDefaultFogBrushSize()
{
    return m_settings->value("display/defaultFogBrushSize", 50).toInt();
}

void SettingsManager::clearAllSettings()
{
    m_settings->clear();
    m_settings->sync();
}

void SettingsManager::syncSettings()
{
    // Force immediate write to disk
    m_settings->sync();
}

// Interaction settings
void SettingsManager::saveWheelZoomEnabled(bool enabled)
{
    m_settings->setValue("interaction/wheelZoomEnabled", enabled);
    m_settings->sync();
}

bool SettingsManager::loadWheelZoomEnabled()
{
    // Default to false to disable touchpad/mousewheel zoom by default
    return m_settings->value("interaction/wheelZoomEnabled", false).toBool();
}

// Per-map grid calibration storage
QString SettingsManager::generateMapKey(const QString& mapPath)
{
    // Create consistent key from file path using hash to avoid path issues
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(mapPath.toUtf8());
    return QString("mapGridCalibration/%1").arg(QString(hash.result().toHex()));
}

void SettingsManager::saveMapGridCalibration(const QString& mapPath, double tvSize, double viewingDistance, int gridSize, const QPointF& gridOffset)
{
    QString key = generateMapKey(mapPath);
    m_settings->beginGroup(key);
    m_settings->setValue("tvSize", tvSize);
    m_settings->setValue("viewingDistance", viewingDistance);
    m_settings->setValue("gridSize", gridSize);
    m_settings->setValue("gridOffsetX", gridOffset.x());
    m_settings->setValue("gridOffsetY", gridOffset.y());
    m_settings->setValue("mapPath", mapPath); // Store original path for debugging
    m_settings->endGroup();
    m_settings->sync();

    DebugConsole::info(QString("Saved map grid calibration for: %1 Size: %2 TV: %3").arg(mapPath).arg(gridSize).arg(tvSize), "Settings");
}

void SettingsManager::loadMapGridCalibration(const QString& mapPath, double& tvSize, double& viewingDistance, int& gridSize, QPointF& gridOffset)
{
    QString key = generateMapKey(mapPath);
    m_settings->beginGroup(key);

    if (m_settings->contains("tvSize")) {
        tvSize = m_settings->value("tvSize", 55.0).toDouble();
        viewingDistance = m_settings->value("viewingDistance", 72.0).toDouble();
        gridSize = m_settings->value("gridSize", 50).toInt();
        gridOffset.setX(m_settings->value("gridOffsetX", 0.0).toDouble());
        gridOffset.setY(m_settings->value("gridOffsetY", 0.0).toDouble());

        DebugConsole::info(QString("Loaded map grid calibration for: %1 Size: %2 TV: %3").arg(mapPath).arg(gridSize).arg(tvSize), "Settings");
    } else {
        // Use global defaults if no per-map settings exist
        tvSize = loadTvSize();
        viewingDistance = loadViewingDistance();
        gridSize = loadGridSize();
        gridOffset = QPointF(0, 0);

        DebugConsole::info(QString("No map-specific calibration found for: %1 Using global defaults").arg(mapPath), "Settings");
    }

    m_settings->endGroup();
}

bool SettingsManager::hasMapGridCalibration(const QString& mapPath)
{
    QString key = generateMapKey(mapPath);
    m_settings->beginGroup(key);
    bool exists = m_settings->contains("tvSize");
    m_settings->endGroup();
    return exists;
}

void SettingsManager::removeMapGridCalibration(const QString& mapPath)
{
    QString key = generateMapKey(mapPath);
    m_settings->remove(key);
    m_settings->sync();
    DebugConsole::info(QString("Removed map grid calibration for: %1").arg(mapPath), "Settings");
}
