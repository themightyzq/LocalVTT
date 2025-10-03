#include "FogAutosaveController.h"
#include "graphics/MapDisplay.h"

#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>

FogAutosaveController::FogAutosaveController(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    m_timer->setInterval(500);
    connect(m_timer, &QTimer::timeout, this, &FogAutosaveController::onAutosaveTimeout);
}

void FogAutosaveController::setMapDisplay(MapDisplay* display)
{
    if (m_display == display) return;
    if (m_display) {
        disconnect(m_display, nullptr, this, nullptr);
    }
    m_display = display;
    if (m_display) {
        connect(m_display, &MapDisplay::fogChanged, this, &FogAutosaveController::onFogChanged);
    }
}

void FogAutosaveController::setCurrentMapPath(const QString& mapPath)
{
    m_currentMapPath = mapPath;
}

void FogAutosaveController::setAutosaveDelayMs(int ms)
{
    m_timer->setInterval(ms);
}

void FogAutosaveController::loadFromFile()
{
    if (!m_display || m_currentMapPath.isEmpty()) return;
    const QString path = fogFilePath();
    QFileInfo info(path);
    if (!info.exists() || !info.isReadable()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QByteArray data = f.readAll();
    f.close();
    if (data.isEmpty()) return;
    m_display->loadFogState(data);
    emit notify(QStringLiteral("Loaded fog state from %1").arg(info.fileName()));
}

void FogAutosaveController::onFogChanged()
{
    m_dirty = true;
    m_timer->start();
}

void FogAutosaveController::onAutosaveTimeout()
{
    if (!m_dirty || !m_display || m_currentMapPath.isEmpty()) return;
    const QString path = fogFilePath();

    const QByteArray data = m_display->saveFogState();
    if (data.isEmpty()) {
        QFile::remove(path);
        emit notify(QStringLiteral("Cleared fog state"));
        m_dirty = false;
        return;
    }

    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        emit notify(QStringLiteral("Failed to save fog state"));
        return;
    }
    const qint64 bytes = f.write(data);
    f.close();
    if (bytes > 0) {
        emit notify(QStringLiteral("Autosaved fog state (%1 bytes)").arg(bytes));
        m_dirty = false;
    } else {
        emit notify(QStringLiteral("Failed to save fog state"));
    }
}

QString FogAutosaveController::fogFilePath() const
{
    if (m_currentMapPath.isEmpty()) return QString();
    return m_currentMapPath + QStringLiteral(".fog");
}

void FogAutosaveController::saveNow()
{
    // Treat as if timer elapsed
    onAutosaveTimeout();
}
