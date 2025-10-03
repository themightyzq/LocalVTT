#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <QPixmap>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QDateTime>

struct ImageCacheKey
{
    QString imagePath;
    QSize targetSize;
    qreal rotation;
    qreal scaleX;
    qreal scaleY;

    bool operator==(const ImageCacheKey& other) const {
        return imagePath == other.imagePath &&
               targetSize == other.targetSize &&
               qFuzzyCompare(rotation, other.rotation) &&
               qFuzzyCompare(scaleX, other.scaleX) &&
               qFuzzyCompare(scaleY, other.scaleY);
    }
};

inline uint qHash(const ImageCacheKey& key, uint seed = 0)
{
    return qHash(key.imagePath, seed) ^
           qHash(key.targetSize.width(), seed) ^
           qHash(key.targetSize.height(), seed) ^
           qHash(static_cast<int>(key.rotation * 100), seed) ^
           qHash(static_cast<int>(key.scaleX * 100), seed) ^
           qHash(static_cast<int>(key.scaleY * 100), seed);
}

struct ImageCacheEntry
{
    QPixmap pixmap;
    QDateTime lastUsed;
    size_t memorySize;
};

class ImageCache
{
public:
    ImageCache();
    ~ImageCache();

    // Get cached pixmap or return null if not found
    QPixmap getCachedPixmap(const ImageCacheKey& key);

    // Store pixmap in cache
    void setCachedPixmap(const ImageCacheKey& key, const QPixmap& pixmap);

    // Clear all cached images
    void clear();

    // Set maximum cache size in bytes (default: 100MB)
    void setMaxCacheSize(size_t maxSize) { m_maxCacheSize = maxSize; }
    size_t getMaxCacheSize() const { return m_maxCacheSize; }

    // Get current cache usage
    size_t getCurrentCacheSize() const { return m_currentCacheSize; }
    int getCacheEntryCount() const { return m_cache.size(); }

    // Enable/disable cache (for testing/debugging)
    void setEnabled(bool enabled) { m_enabled = enabled; if (!enabled) clear(); }
    bool isEnabled() const { return m_enabled; }

private:
    void evictLRU();
    void updateEntryUsage(const ImageCacheKey& key);
    size_t calculatePixmapSize(const QPixmap& pixmap);

    QHash<ImageCacheKey, ImageCacheEntry> m_cache;
    QList<ImageCacheKey> m_lruOrder;
    size_t m_maxCacheSize;
    size_t m_currentCacheSize;
    bool m_enabled;
    mutable QMutex m_cacheMutex;
};

// Singleton instance for global access
class ImageCacheManager
{
public:
    static ImageCache& instance();

private:
    static ImageCache* s_instance;
    static QMutex s_instanceMutex;
};

#endif // IMAGECACHE_H