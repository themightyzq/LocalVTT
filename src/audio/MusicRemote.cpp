#include "audio/MusicRemote.h"
#include "utils/DebugConsole.h"
#include <QDesktopServices>
#include <QUrl>

MusicRemote::MusicRemote(QObject* parent)
    : QObject(parent)
    , m_pollTimer(new QTimer(this))
{
    connect(m_pollTimer, &QTimer::timeout, this, &MusicRemote::pollNowPlaying);
}

MusicRemote::~MusicRemote()
{
    m_pollTimer->stop();
}

MusicRemote::NowPlaying MusicRemote::getNowPlaying() const
{
    return platformGetNowPlaying();
}

bool MusicRemote::isAvailable() const
{
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

void MusicRemote::playPause()
{
    platformPlayPause();
}

void MusicRemote::nextTrack()
{
    platformNextTrack();
}

void MusicRemote::previousTrack()
{
    platformPreviousTrack();
}

void MusicRemote::setVolume(qreal volume)
{
    platformSetVolume(qBound(0.0, volume, 1.0));
}

void MusicRemote::openMusicURL(const QString& url)
{
    if (url.isEmpty()) return;
    QDesktopServices::openUrl(QUrl(url));
    DebugConsole::info(QString("MusicRemote: opening URL %1").arg(url), "Audio");
}

void MusicRemote::startPolling(int intervalMs)
{
    m_pollTimer->start(intervalMs);
}

void MusicRemote::stopPolling()
{
    m_pollTimer->stop();
}

void MusicRemote::pollNowPlaying()
{
    NowPlaying current = platformGetNowPlaying();

    if (current.title != m_lastNowPlaying.title ||
        current.artist != m_lastNowPlaying.artist ||
        current.isPlaying != m_lastNowPlaying.isPlaying) {
        m_lastNowPlaying = current;
        emit nowPlayingChanged(current);
    }
}

// Platform-specific implementations follow in MusicRemote_mac.mm (macOS)
// or stubbed out below for non-macOS platforms.

#ifndef __APPLE__

MusicRemote::NowPlaying MusicRemote::platformGetNowPlaying() const
{
    return NowPlaying{};
}

void MusicRemote::platformPlayPause() {}
void MusicRemote::platformNextTrack() {}
void MusicRemote::platformPreviousTrack() {}
void MusicRemote::platformSetVolume(qreal) {}

#endif
