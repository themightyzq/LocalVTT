#ifndef AMBIENTPLAYER_H
#define AMBIENTPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QUrl>

class AmbientPlayer : public QObject {
    Q_OBJECT
public:
    explicit AmbientPlayer(QObject* parent = nullptr);
    ~AmbientPlayer();

    void play(const QString& filePath, int fadeInMs = 1000);
    void stop(int fadeOutMs = 1000);
    void crossfadeTo(const QString& filePath, int durationMs = 2000);
    void setVolume(qreal volume);  // 0.0 - 1.0
    qreal volume() const { return m_targetVolume; }
    bool isPlaying() const;
    QString currentTrack() const { return m_currentTrack; }

signals:
    void trackChanged(const QString& filePath);
    void playbackStateChanged(bool playing);
    void volumeChanged(qreal volume);

private slots:
    void onCrossfadeTick();
    void onFadeOutFinished();

private:
    struct Deck {
        QMediaPlayer* player = nullptr;
        QAudioOutput* output = nullptr;
    };

    void setupDeck(Deck& deck);
    Deck& activeDeck() { return m_decks[m_activeDeckIndex]; }
    Deck& inactiveDeck() { return m_decks[1 - m_activeDeckIndex]; }

    Deck m_decks[2];
    int m_activeDeckIndex = 0;

    qreal m_targetVolume = 0.7;
    QString m_currentTrack;

    // Crossfade state
    QTimer* m_crossfadeTimer = nullptr;
    int m_crossfadeDurationMs = 2000;
    int m_crossfadeElapsedMs = 0;
    bool m_isCrossfading = false;
    static constexpr int CROSSFADE_TICK_MS = 30;

    // Fade-out-only state (for stop())
    QTimer* m_fadeOutTimer = nullptr;
    int m_fadeOutDurationMs = 1000;
    int m_fadeOutElapsedMs = 0;
    qreal m_fadeOutStartVolume = 0.0;
};

#endif // AMBIENTPLAYER_H
