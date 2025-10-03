#ifndef DEBUGCONSOLE_H
#define DEBUGCONSOLE_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QTimer>
#include <QElapsedTimer>
#include <QOpenGLContext>
#include <QGlobalStatic>

class DebugConsoleWidget;

struct DebugMessage
{
    QString timestamp;
    QString level;
    QString message;
    QString category;
};

struct PerformanceMetrics
{
    qreal fps = 0.0;
    qint64 memoryUsage = 0;
    qint64 lastLoadTime = 0;
    int totalLoads = 0;
    qreal averageLoadTime = 0.0;
};

struct SystemInfo
{
    QString qtVersion;
    QString openglVersion;
    QString openglRenderer;
    QString platformName;
    QStringList availablePlugins;
    bool openglSupported = false;
    QString cpuArchitecture;
    qint64 totalMemory = 0;
};

class DebugConsole : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        Info,
        Warning,
        Error,
        Performance,
        System,
        VTT
    };

    static DebugConsole* instance();
    
    static void info(const QString& message, const QString& category = "General");
    static void warning(const QString& message, const QString& category = "General");
    static void error(const QString& message, const QString& category = "General");
    static void performance(const QString& message, const QString& category = "Performance");
    static void system(const QString& message, const QString& category = "System");
    static void vtt(const QString& message, const QString& category = "VTT");

    static void recordLoadTime(qint64 milliseconds);
    static void updateFPS(qreal fps);
    static void updateMemoryUsage(qint64 bytes);

    void setWidget(DebugConsoleWidget* widget);
    
    const QList<DebugMessage>& getMessages() const { return m_messages; }
    const PerformanceMetrics& getMetrics() const { return m_metrics; }
    const SystemInfo& getSystemInfo() const { return m_systemInfo; }

    void clearMessages();

signals:
    void messageAdded(const DebugMessage& message);
    void metricsUpdated(const PerformanceMetrics& metrics);

private slots:
    void updateSystemMetrics();

public:
    explicit DebugConsole(QObject* parent = nullptr);
    ~DebugConsole() = default;

private:
    DebugConsole(const DebugConsole&) = delete;
    DebugConsole& operator=(const DebugConsole&) = delete;

    void log(LogLevel level, const QString& message, const QString& category);
    QString levelToString(LogLevel level) const;
    void collectSystemInfo();
    void collectOpenGLInfo();
    void collectQtPluginInfo();
    qint64 getCurrentMemoryUsage() const;

    QList<DebugMessage> m_messages;
    PerformanceMetrics m_metrics;
    SystemInfo m_systemInfo;
    DebugConsoleWidget* m_widget;
    
    QMutex m_messagesMutex;
    QTimer* m_metricsTimer;
    QElapsedTimer m_fpsTimer;
    int m_frameCount;
    bool m_systemInfoCollected = false;

    static const int MAX_MESSAGES = 1000;
    static const int METRICS_UPDATE_INTERVAL = 1000;
};

#endif