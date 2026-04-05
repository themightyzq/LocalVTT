#ifndef MUSICREMOTE_H
#define MUSICREMOTE_H

#include <QObject>
#include <QImage>
#include <QTimer>

class MusicRemote : public QObject {
    Q_OBJECT
public:
    explicit MusicRemote(QObject* parent = nullptr);
    ~MusicRemote();

    struct NowPlaying {
        QString title;
        QString artist;
        QString album;
        QImage artwork;
        qreal duration = 0.0;
        qreal position = 0.0;
        bool isPlaying = false;
    };

    NowPlaying getNowPlaying() const;
    bool isAvailable() const;

public slots:
    void playPause();
    void nextTrack();
    void previousTrack();
    void setVolume(qreal volume);
    void openMusicURL(const QString& url);

    void startPolling(int intervalMs = 2000);
    void stopPolling();

signals:
    void nowPlayingChanged(const MusicRemote::NowPlaying& info);

private slots:
    void pollNowPlaying();

private:
    QTimer* m_pollTimer;
    NowPlaying m_lastNowPlaying;

    // Platform-specific implementations
    NowPlaying platformGetNowPlaying() const;
    void platformPlayPause();
    void platformNextTrack();
    void platformPreviousTrack();
    void platformSetVolume(qreal volume);
};

#endif // MUSICREMOTE_H
