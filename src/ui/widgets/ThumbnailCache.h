#ifndef THUMBNAILCACHE_H
#define THUMBNAILCACHE_H

#include <QObject>
#include <QCache>
#include <QPixmap>
#include <QHash>
#include <QMutex>
#include <QRunnable>

class QThreadPool;

// Async thumbnail generation with memory and disk caching
// Uses QThreadPool for background loading without blocking UI
class ThumbnailCache : public QObject
{
    Q_OBJECT

public:
    static ThumbnailCache& instance();

    // Request a thumbnail - returns immediately with placeholder if not cached
    // Emits thumbnailReady when the real thumbnail is available
    QPixmap getThumbnail(const QString& filePath);

    // Check if thumbnail is cached (memory or disk)
    bool hasThumbnail(const QString& filePath) const;

    // Clear caches
    void clearMemoryCache();
    void clearDiskCache();
    void clearAll();

    // Configuration
    void setThumbnailSize(int size);
    int thumbnailSize() const { return m_thumbnailSize; }

    void setMemoryCacheSize(int sizeInMB);
    int memoryCacheSize() const { return m_memoryCacheSizeMB; }

signals:
    void thumbnailReady(const QString& filePath, const QPixmap& thumbnail);

private:
    ThumbnailCache();
    ~ThumbnailCache();
    ThumbnailCache(const ThumbnailCache&) = delete;
    ThumbnailCache& operator=(const ThumbnailCache&) = delete;

    QString getCacheDirectory() const;
    QString getCacheFilePath(const QString& filePath) const;
    QString generateCacheKey(const QString& filePath) const;

    QPixmap loadFromDiskCache(const QString& filePath);
    void saveToDiskCache(const QString& filePath, const QPixmap& thumbnail);

    void requestThumbnailGeneration(const QString& filePath);

    QCache<QString, QPixmap> m_memoryCache;
    QHash<QString, bool> m_pendingRequests;
    QMutex m_mutex;
    QThreadPool* m_threadPool;

    int m_thumbnailSize = 128;
    int m_memoryCacheSizeMB = 50;

    QPixmap m_placeholderPixmap;

    friend class ThumbnailWorker;
};

// Worker for async thumbnail generation
class ThumbnailWorker : public QRunnable
{
public:
    ThumbnailWorker(const QString& filePath, ThumbnailCache* cache);
    void run() override;

private:
    QString m_filePath;
    ThumbnailCache* m_cache;
};

#endif // THUMBNAILCACHE_H
