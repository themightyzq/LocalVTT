#include "ThumbnailCache.h"
#include "utils/VTTLoader.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QThreadPool>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QDebug>

ThumbnailCache& ThumbnailCache::instance()
{
    static ThumbnailCache instance;
    return instance;
}

ThumbnailCache::ThumbnailCache()
    : QObject(nullptr)
    , m_threadPool(new QThreadPool(this))
{
    // Configure thread pool - limit concurrent thumbnail generation
    m_threadPool->setMaxThreadCount(2);

    // Set memory cache size (estimate ~50KB per 128x128 RGBA thumbnail)
    // 50MB / 50KB = ~1000 thumbnails
    m_memoryCache.setMaxCost(m_memoryCacheSizeMB * 1024);

    // Create placeholder pixmap
    m_placeholderPixmap = QPixmap(m_thumbnailSize, m_thumbnailSize);
    m_placeholderPixmap.fill(QColor(0x2D, 0x2D, 0x2D));
    QPainter painter(&m_placeholderPixmap);
    painter.setPen(QColor(0x60, 0x60, 0x60));
    painter.drawRect(0, 0, m_thumbnailSize - 1, m_thumbnailSize - 1);
    // Draw a simple map icon
    painter.setPen(QColor(0x80, 0x80, 0x80));
    int margin = m_thumbnailSize / 4;
    painter.drawRect(margin, margin, m_thumbnailSize - 2 * margin, m_thumbnailSize - 2 * margin);
    painter.end();

    // Ensure cache directory exists
    QDir().mkpath(getCacheDirectory());
}

ThumbnailCache::~ThumbnailCache()
{
    m_threadPool->waitForDone(5000);
}

QString ThumbnailCache::getCacheDirectory() const
{
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return cacheDir + "/thumbnails";
}

QString ThumbnailCache::generateCacheKey(const QString& filePath) const
{
    QFileInfo info(filePath);
    QString key = filePath + QString::number(info.size()) + info.lastModified().toString(Qt::ISODate);
    QByteArray hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5);
    return hash.toHex();
}

QString ThumbnailCache::getCacheFilePath(const QString& filePath) const
{
    QString cacheKey = generateCacheKey(filePath);
    return getCacheDirectory() + "/" + cacheKey + ".png";
}

QPixmap ThumbnailCache::getThumbnail(const QString& filePath)
{
    QMutexLocker locker(&m_mutex);

    // Check memory cache first
    QPixmap* cached = m_memoryCache.object(filePath);
    if (cached) {
        return *cached;
    }

    // Check disk cache
    QPixmap diskCached = loadFromDiskCache(filePath);
    if (!diskCached.isNull()) {
        // Add to memory cache (estimate cost as KB)
        int cost = (diskCached.width() * diskCached.height() * 4) / 1024;
        m_memoryCache.insert(filePath, new QPixmap(diskCached), qMax(cost, 1));
        return diskCached;
    }

    // Not cached - request async generation if not already pending
    if (!m_pendingRequests.contains(filePath)) {
        m_pendingRequests.insert(filePath, true);
        locker.unlock();
        requestThumbnailGeneration(filePath);
    }

    return m_placeholderPixmap;
}

bool ThumbnailCache::hasThumbnail(const QString& filePath) const
{
    QMutexLocker locker(&const_cast<QMutex&>(m_mutex));

    if (m_memoryCache.contains(filePath)) {
        return true;
    }

    QString cachePath = getCacheFilePath(filePath);
    return QFile::exists(cachePath);
}

QPixmap ThumbnailCache::loadFromDiskCache(const QString& filePath)
{
    QString cachePath = getCacheFilePath(filePath);
    if (QFile::exists(cachePath)) {
        QPixmap pixmap;
        if (pixmap.load(cachePath)) {
            return pixmap;
        }
    }
    return QPixmap();
}

void ThumbnailCache::saveToDiskCache(const QString& filePath, const QPixmap& thumbnail)
{
    QString cachePath = getCacheFilePath(filePath);
    thumbnail.save(cachePath, "PNG");
}

void ThumbnailCache::requestThumbnailGeneration(const QString& filePath)
{
    ThumbnailWorker* worker = new ThumbnailWorker(filePath, this);
    m_threadPool->start(worker);
}

void ThumbnailCache::clearMemoryCache()
{
    QMutexLocker locker(&m_mutex);
    m_memoryCache.clear();
}

void ThumbnailCache::clearDiskCache()
{
    QMutexLocker locker(&m_mutex);
    QDir cacheDir(getCacheDirectory());
    if (cacheDir.exists()) {
        for (const QString& file : cacheDir.entryList(QStringList() << "*.png", QDir::Files)) {
            cacheDir.remove(file);
        }
    }
}

void ThumbnailCache::clearAll()
{
    clearMemoryCache();
    clearDiskCache();
}

void ThumbnailCache::setThumbnailSize(int size)
{
    m_thumbnailSize = qBound(64, size, 256);
}

void ThumbnailCache::setMemoryCacheSize(int sizeInMB)
{
    m_memoryCacheSizeMB = qBound(10, sizeInMB, 200);
    m_memoryCache.setMaxCost(m_memoryCacheSizeMB * 1024);
}

// ThumbnailWorker implementation
ThumbnailWorker::ThumbnailWorker(const QString& filePath, ThumbnailCache* cache)
    : m_filePath(filePath)
    , m_cache(cache)
{
    setAutoDelete(true);
}

void ThumbnailWorker::run()
{
    QImage image;
    QString ext = QFileInfo(m_filePath).suffix().toLower();
    int targetSize = m_cache->m_thumbnailSize;

    // Handle VTT formats specially - they have embedded images
    if (ext == "dd2vtt" || ext == "uvtt" || ext == "df2vtt") {
        VTTLoader::VTTData data = VTTLoader::loadVTT(m_filePath);
        if (data.isValid) {
            image = data.mapImage;
        }
    } else {
        // Use QImageReader for efficient loading of large images
        // This allows reading at a scaled size without loading the full image into memory
        QImageReader reader(m_filePath);
        reader.setAutoTransform(true);

        QSize originalSize = reader.size();
        if (originalSize.isValid()) {
            // Calculate the scale factor needed for thumbnail
            // We want to load at roughly 2x thumbnail size for quality, then scale down
            int maxDimension = qMax(originalSize.width(), originalSize.height());
            int loadSize = targetSize * 2;  // Load at 2x for quality

            if (maxDimension > loadSize) {
                // Scale down during load to avoid memory issues
                qreal scaleFactor = static_cast<qreal>(loadSize) / maxDimension;
                QSize scaledSize(
                    static_cast<int>(originalSize.width() * scaleFactor),
                    static_cast<int>(originalSize.height() * scaleFactor)
                );
                reader.setScaledSize(scaledSize);
            }
        }

        image = reader.read();

        if (image.isNull()) {
            qDebug() << "ThumbnailCache: Failed to load image:" << m_filePath
                     << "Error:" << reader.errorString();
        }
    }

    if (image.isNull()) {
        // Remove from pending
        QMutexLocker locker(&m_cache->m_mutex);
        m_cache->m_pendingRequests.remove(m_filePath);
        return;
    }

    // Scale to final thumbnail size
    int size = targetSize;
    QImage scaled = image.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Center in square pixmap with background
    QPixmap thumbnail(size, size);
    thumbnail.fill(QColor(0x2D, 0x2D, 0x2D));
    QPainter painter(&thumbnail);
    int x = (size - scaled.width()) / 2;
    int y = (size - scaled.height()) / 2;
    painter.drawImage(x, y, scaled);
    painter.end();

    // Save to disk cache
    m_cache->saveToDiskCache(m_filePath, thumbnail);

    // Update memory cache
    {
        QMutexLocker locker(&m_cache->m_mutex);
        int cost = (thumbnail.width() * thumbnail.height() * 4) / 1024;
        m_cache->m_memoryCache.insert(m_filePath, new QPixmap(thumbnail), qMax(cost, 1));
        m_cache->m_pendingRequests.remove(m_filePath);
    }

    // Signal completion on main thread
    QMetaObject::invokeMethod(m_cache, "thumbnailReady",
                              Qt::QueuedConnection,
                              Q_ARG(QString, m_filePath),
                              Q_ARG(QPixmap, thumbnail));
}
