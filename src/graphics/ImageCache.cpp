#include "graphics/ImageCache.h"
#include <QMutexLocker>

ImageCache* ImageCacheManager::s_instance = nullptr;
QMutex ImageCacheManager::s_instanceMutex;

ImageCache::ImageCache()
    : m_maxCacheSize(100 * 1024 * 1024) // 100MB default
    , m_currentCacheSize(0)
    , m_enabled(true)
{
}

ImageCache::~ImageCache()
{
    clear();
}

QPixmap ImageCache::getCachedPixmap(const ImageCacheKey& key)
{
    if (!m_enabled) {
        return QPixmap();
    }

    QMutexLocker locker(&m_cacheMutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // Update last used time and move to front of LRU list
        it->lastUsed = QDateTime::currentDateTime();
        updateEntryUsage(key);
        return it->pixmap;
    }

    return QPixmap(); // Not found
}

void ImageCache::setCachedPixmap(const ImageCacheKey& key, const QPixmap& pixmap)
{
    if (!m_enabled || pixmap.isNull()) {
        return;
    }

    QMutexLocker locker(&m_cacheMutex);

    size_t pixmapSize = calculatePixmapSize(pixmap);

    // Check if adding this would exceed cache size
    if (pixmapSize > m_maxCacheSize) {
        // Pixmap too large for cache
        return;
    }

    // Remove existing entry if it exists
    auto existing = m_cache.find(key);
    if (existing != m_cache.end()) {
        m_currentCacheSize -= existing->memorySize;
        m_lruOrder.removeAll(key);
        m_cache.erase(existing);
    }

    // Evict entries until we have space
    while (m_currentCacheSize + pixmapSize > m_maxCacheSize && !m_cache.isEmpty()) {
        evictLRU();
    }

    // Add new entry
    ImageCacheEntry entry;
    entry.pixmap = pixmap;
    entry.lastUsed = QDateTime::currentDateTime();
    entry.memorySize = pixmapSize;

    m_cache[key] = entry;
    m_lruOrder.prepend(key);
    m_currentCacheSize += pixmapSize;
}

void ImageCache::clear()
{
    QMutexLocker locker(&m_cacheMutex);

    m_cache.clear();
    m_lruOrder.clear();
    m_currentCacheSize = 0;
}

void ImageCache::evictLRU()
{
    if (m_lruOrder.isEmpty()) {
        return;
    }

    // Remove least recently used entry
    ImageCacheKey oldestKey = m_lruOrder.last();
    auto it = m_cache.find(oldestKey);

    if (it != m_cache.end()) {
        m_currentCacheSize -= it->memorySize;
        m_cache.erase(it);
    }

    m_lruOrder.removeLast();
}

void ImageCache::updateEntryUsage(const ImageCacheKey& key)
{
    // Move key to front of LRU list
    m_lruOrder.removeAll(key);
    m_lruOrder.prepend(key);
}

size_t ImageCache::calculatePixmapSize(const QPixmap& pixmap)
{
    // Estimate memory usage: width * height * bytes per pixel (4 for ARGB32)
    return static_cast<size_t>(pixmap.width()) * pixmap.height() * 4;
}

ImageCache& ImageCacheManager::instance()
{
    QMutexLocker locker(&s_instanceMutex);

    if (!s_instance) {
        s_instance = new ImageCache();
    }

    return *s_instance;
}