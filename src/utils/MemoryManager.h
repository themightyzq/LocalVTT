#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QObject>
#include <QImage>
#include <atomic>

class MemoryManager : public QObject
{
    Q_OBJECT

public:
    static MemoryManager& instance();

    // Memory tracking
    void reportImageLoaded(const QImage& image);
    void reportImageReleased(const QImage& image);
    qint64 getCurrentMemoryUsage() const { return m_currentMemoryUsage; }
    qint64 getMaxMemoryLimit() const { return m_maxMemoryLimit; }
    bool isUnderMemoryPressure() const;

    // Memory optimization hints
    bool shouldCompressImage(const QImage& image) const;
    bool shouldReleaseInactiveTabs() const;

    // Settings
    void setMaxMemoryLimit(qint64 bytes) { m_maxMemoryLimit = bytes; }

signals:
    void memoryPressureDetected();
    void memoryPressureRelieved();

private:
    MemoryManager();
    ~MemoryManager() = default;

    static qint64 calculateImageMemory(const QImage& image);

    std::atomic<qint64> m_currentMemoryUsage{0};
    std::atomic<qint64> m_maxMemoryLimit{150 * 1024 * 1024};  // 150MB default limit

    // Thresholds
    static constexpr double PRESSURE_THRESHOLD = 0.90;  // 90% of limit
    static constexpr double RELEASE_THRESHOLD = 0.95;   // 95% of limit
    static constexpr qint64 MIN_IMAGE_SIZE_TO_COMPRESS = 5 * 1024 * 1024;  // 5MB
};

#endif // MEMORYMANAGER_H