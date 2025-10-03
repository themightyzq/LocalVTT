#include "MemoryManager.h"
#include "DebugConsole.h"

MemoryManager& MemoryManager::instance()
{
    static MemoryManager instance;
    return instance;
}

MemoryManager::MemoryManager()
    : QObject(nullptr)
{
}

void MemoryManager::reportImageLoaded(const QImage& image)
{
    if (image.isNull()) return;

    qint64 imageMemory = calculateImageMemory(image);
    qint64 oldUsage = m_currentMemoryUsage.fetch_add(imageMemory);
    qint64 newUsage = oldUsage + imageMemory;

    DebugConsole::performance(QString("Image loaded: +%1 KB (Total: %2 MB / %3 MB)")
        .arg(imageMemory / 1024)
        .arg(newUsage / (1024 * 1024))
        .arg(m_maxMemoryLimit / (1024 * 1024)), "Memory");

    if (isUnderMemoryPressure() && oldUsage < (m_maxMemoryLimit * PRESSURE_THRESHOLD)) {
        emit memoryPressureDetected();
    }
}

void MemoryManager::reportImageReleased(const QImage& image)
{
    if (image.isNull()) return;

    qint64 imageMemory = calculateImageMemory(image);
    qint64 oldUsage = m_currentMemoryUsage.fetch_sub(imageMemory);
    qint64 newUsage = oldUsage - imageMemory;

    DebugConsole::performance(QString("Image released: -%1 KB (Total: %2 MB / %3 MB)")
        .arg(imageMemory / 1024)
        .arg(newUsage / (1024 * 1024))
        .arg(m_maxMemoryLimit / (1024 * 1024)), "Memory");

    if (!isUnderMemoryPressure() && oldUsage >= (m_maxMemoryLimit * PRESSURE_THRESHOLD)) {
        emit memoryPressureRelieved();
    }
}

bool MemoryManager::isUnderMemoryPressure() const
{
    return m_currentMemoryUsage >= (m_maxMemoryLimit * PRESSURE_THRESHOLD);
}

bool MemoryManager::shouldCompressImage(const QImage& image) const
{
    if (image.isNull()) return false;

    // Compress large images or when under memory pressure
    qint64 imageMemory = calculateImageMemory(image);
    return (imageMemory >= MIN_IMAGE_SIZE_TO_COMPRESS) || isUnderMemoryPressure();
}

bool MemoryManager::shouldReleaseInactiveTabs() const
{
    // Release inactive tabs when we're using more than 95% of limit
    return m_currentMemoryUsage >= (m_maxMemoryLimit * RELEASE_THRESHOLD);
}

qint64 MemoryManager::calculateImageMemory(const QImage& image)
{
    if (image.isNull()) return 0;

    // Calculate actual memory usage based on format
    qint64 bytesPerPixel = 4;  // Default ARGB32

    switch (image.format()) {
        case QImage::Format_RGB888:
            bytesPerPixel = 3;
            break;
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
            bytesPerPixel = 4;
            break;
        case QImage::Format_RGB16:
        case QImage::Format_RGB555:
        case QImage::Format_RGB444:
            bytesPerPixel = 2;
            break;
        case QImage::Format_Indexed8:
        case QImage::Format_Grayscale8:
            bytesPerPixel = 1;
            break;
        default:
            bytesPerPixel = 4;  // Conservative estimate
            break;
    }

    // Include overhead for QImage structure and potential alignment
    qint64 overhead = 1024;  // Approximate overhead
    return (image.width() * image.height() * bytesPerPixel) + overhead;
}