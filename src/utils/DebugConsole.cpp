#include "DebugConsole.h"
#include "ui/DebugConsoleWidget.h"
#include <QMutexLocker>
#include <QCoreApplication>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSysInfo>
#include <QDir>
#include <QLibraryInfo>
#include <QProcess>

// Use Q_GLOBAL_STATIC for thread-safe, lazy initialization without recursion issues
Q_GLOBAL_STATIC(DebugConsole, g_debugConsoleInstance)

DebugConsole* DebugConsole::instance()
{
    return g_debugConsoleInstance();
}

DebugConsole::DebugConsole(QObject* parent)
    : QObject(parent)
    , m_widget(nullptr)
    , m_frameCount(0)
{
    // Don't collect system info during construction to avoid recursive initialization
    // This will be done on first actual use instead

    m_metricsTimer = new QTimer(this);
    connect(m_metricsTimer, &QTimer::timeout, this, &DebugConsole::updateSystemMetrics);
    m_metricsTimer->start(METRICS_UPDATE_INTERVAL);

    m_fpsTimer.start();
}

void DebugConsole::info(const QString& message, const QString& category)
{
    if (auto* console = instance()) {
        console->log(Info, message, category);
    }
}

void DebugConsole::warning(const QString& message, const QString& category)
{
    if (auto* console = instance()) {
        console->log(Warning, message, category);
    }
}

void DebugConsole::error(const QString& message, const QString& category)
{
    if (auto* console = instance()) {
        console->log(Error, message, category);
    }
}

void DebugConsole::performance(const QString& message, const QString& category)
{
    if (auto* console = instance()) {
        console->log(Performance, message, category);
    }
}

void DebugConsole::system(const QString& message, const QString& category)
{
    if (auto* console = instance()) {
        console->log(System, message, category);
    }
}

void DebugConsole::vtt(const QString& message, const QString& category)
{
    if (auto* console = instance()) {
        console->log(VTT, message, category);
    }
}

void DebugConsole::recordLoadTime(qint64 milliseconds)
{
    auto* console = instance();
    if (!console) return;

    console->m_metrics.lastLoadTime = milliseconds;
    console->m_metrics.totalLoads++;

    qreal totalTime = console->m_metrics.averageLoadTime * (console->m_metrics.totalLoads - 1) + milliseconds;
    console->m_metrics.averageLoadTime = totalTime / console->m_metrics.totalLoads;

    emit console->metricsUpdated(console->m_metrics);

    performance(QString("Load completed in %1ms (avg: %2ms)")
               .arg(milliseconds)
               .arg(qRound(console->m_metrics.averageLoadTime)), "Loading");
}

void DebugConsole::updateFPS(qreal fps)
{
    auto* console = instance();
    if (!console) return;

    console->m_metrics.fps = fps;
    emit console->metricsUpdated(console->m_metrics);
}

void DebugConsole::updateMemoryUsage(qint64 bytes)
{
    auto* console = instance();
    if (!console) return;

    console->m_metrics.memoryUsage = bytes;
    emit console->metricsUpdated(console->m_metrics);
}

void DebugConsole::setWidget(DebugConsoleWidget* widget)
{
    m_widget = widget;
    if (m_widget) {
        for (const DebugMessage& message : m_messages) {
            m_widget->addMessage(message);
        }
    }
}

void DebugConsole::clearMessages()
{
    QMutexLocker locker(&m_messagesMutex);
    m_messages.clear();
    if (m_widget) {
        m_widget->clearMessages();
    }
}

void DebugConsole::log(LogLevel level, const QString& message, const QString& category)
{
    // Lazy initialize system info on first use
    if (!m_systemInfoCollected) {
        m_systemInfoCollected = true;
        collectSystemInfo();
    }

    DebugMessage debugMessage;
    debugMessage.timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    debugMessage.level = levelToString(level);
    debugMessage.message = message;
    debugMessage.category = category;
    
    QMutexLocker locker(&m_messagesMutex);
    
    if (m_messages.size() >= MAX_MESSAGES) {
        m_messages.removeFirst();
    }
    
    m_messages.append(debugMessage);
    
    if (m_widget) {
        m_widget->addMessage(debugMessage);
    }
    
    emit messageAdded(debugMessage);
}

QString DebugConsole::levelToString(LogLevel level) const
{
    switch (level) {
        case Info: return "INFO";
        case Warning: return "WARN";
        case Error: return "ERROR";
        case Performance: return "PERF";
        case System: return "SYS";
        case VTT: return "VTT";
        default: return "UNKNOWN";
    }
}

void DebugConsole::updateSystemMetrics()
{
    m_metrics.memoryUsage = getCurrentMemoryUsage();
    
    m_frameCount++;
    qint64 elapsed = m_fpsTimer.elapsed();
    if (elapsed >= 1000) {
        m_metrics.fps = (m_frameCount * 1000.0) / elapsed;
        m_frameCount = 0;
        m_fpsTimer.restart();
    }
    
    emit metricsUpdated(m_metrics);
}

void DebugConsole::collectSystemInfo()
{
    m_systemInfo.qtVersion = QT_VERSION_STR;
    m_systemInfo.platformName = QSysInfo::productType() + " " + QSysInfo::productVersion();
    m_systemInfo.cpuArchitecture = QSysInfo::currentCpuArchitecture();

    collectOpenGLInfo();
    collectQtPluginInfo();

    // Don't call static methods during initialization to avoid recursion
    // These will be logged when the console is actually used
    /*
    system(QString("Qt Version: %1").arg(m_systemInfo.qtVersion), "System");
    system(QString("Platform: %1").arg(m_systemInfo.platformName), "System");
    system(QString("CPU Architecture: %1").arg(m_systemInfo.cpuArchitecture), "System");

    if (m_systemInfo.openglSupported) {
        system(QString("OpenGL Version: %1").arg(m_systemInfo.openglVersion), "OpenGL");
        system(QString("OpenGL Renderer: %1").arg(m_systemInfo.openglRenderer), "OpenGL");
    } else {
        error("OpenGL not supported or context not available", "OpenGL");
    }
    */
}

void DebugConsole::collectOpenGLInfo()
{
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (context && context->isValid()) {
        m_systemInfo.openglSupported = true;
        
        QOpenGLFunctions* functions = context->functions();
        if (functions) {
            const char* version = reinterpret_cast<const char*>(functions->glGetString(GL_VERSION));
            const char* renderer = reinterpret_cast<const char*>(functions->glGetString(GL_RENDERER));
            
            if (version) m_systemInfo.openglVersion = QString::fromLatin1(version);
            if (renderer) m_systemInfo.openglRenderer = QString::fromLatin1(renderer);
        }
    } else {
        m_systemInfo.openglSupported = false;
        m_systemInfo.openglVersion = "Not available";
        m_systemInfo.openglRenderer = "Not available";
    }
}

void DebugConsole::collectQtPluginInfo()
{
    QDir pluginsDir(QLibraryInfo::path(QLibraryInfo::PluginsPath));
    if (pluginsDir.exists()) {
        QStringList subdirs = pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& subdir : subdirs) {
            QDir categoryDir(pluginsDir.absoluteFilePath(subdir));
            QStringList plugins = categoryDir.entryList(QDir::Files);
            for (const QString& plugin : plugins) {
                if (plugin.endsWith(".dylib") || plugin.endsWith(".dll") || plugin.endsWith(".so")) {
                    m_systemInfo.availablePlugins.append(QString("%1/%2").arg(subdir, plugin));
                }
            }
        }
    }

    // Don't log during initialization - this avoids recursive initialization
    // The plugin count will be available in m_systemInfo.availablePlugins.size()
}

qint64 DebugConsole::getCurrentMemoryUsage() const
{
#ifdef Q_OS_MACOS
    QProcess process;
    process.start("ps", QStringList() << "-o" << "rss=" << "-p" << QString::number(QCoreApplication::applicationPid()));
    process.waitForFinished(1000);
    
    if (process.exitCode() == 0) {
        QString output = process.readAllStandardOutput().trimmed();
        bool ok;
        qint64 rss = output.toLongLong(&ok);
        if (ok) {
            return rss * 1024;
        }
    }
#endif
    
    return 0;
}