#include "audio/AmbientPlayer.h"
#include "utils/DebugConsole.h"
#include <QFileInfo>

AmbientPlayer::AmbientPlayer(QObject* parent)
    : QObject(parent)
{
    setupDeck(m_decks[0]);
    setupDeck(m_decks[1]);

    m_crossfadeTimer = new QTimer(this);
    m_crossfadeTimer->setInterval(CROSSFADE_TICK_MS);
    connect(m_crossfadeTimer, &QTimer::timeout, this, &AmbientPlayer::onCrossfadeTick);

    m_fadeOutTimer = new QTimer(this);
    m_fadeOutTimer->setInterval(CROSSFADE_TICK_MS);
    connect(m_fadeOutTimer, &QTimer::timeout, this, &AmbientPlayer::onFadeOutFinished);
}

AmbientPlayer::~AmbientPlayer()
{
    m_crossfadeTimer->stop();
    m_fadeOutTimer->stop();
    for (auto& deck : m_decks) {
        if (deck.player) {
            deck.player->stop();
        }
    }
}

void AmbientPlayer::setupDeck(Deck& deck)
{
    deck.player = new QMediaPlayer(this);
    deck.output = new QAudioOutput(this);
    deck.player->setAudioOutput(deck.output);
    deck.player->setLoops(QMediaPlayer::Infinite);
    deck.output->setVolume(0.0);
}

void AmbientPlayer::play(const QString& filePath, int fadeInMs)
{
    if (filePath.isEmpty()) {
        stop(fadeInMs);
        return;
    }

    // If something is already playing, crossfade to the new track
    if (isPlaying()) {
        crossfadeTo(filePath, fadeInMs);
        return;
    }

    // Fresh start — use active deck
    Deck& deck = activeDeck();
    deck.player->setSource(QUrl::fromLocalFile(filePath));
    deck.output->setVolume(0.0);
    deck.player->play();

    m_currentTrack = filePath;
    emit trackChanged(filePath);
    emit playbackStateChanged(true);

    // Fade in
    m_isCrossfading = true;
    m_crossfadeDurationMs = fadeInMs;
    m_crossfadeElapsedMs = 0;
    // During fade-in, inactive deck is silent (nothing to fade out)
    inactiveDeck().output->setVolume(0.0);
    m_crossfadeTimer->start();

    DebugConsole::info(QString("Ambient: playing %1").arg(QFileInfo(filePath).fileName()), "Audio");
}

void AmbientPlayer::stop(int fadeOutMs)
{
    if (!isPlaying()) return;

    if (fadeOutMs <= 0) {
        activeDeck().player->stop();
        activeDeck().output->setVolume(0.0);
        m_currentTrack.clear();
        emit playbackStateChanged(false);
        emit trackChanged(QString());
        return;
    }

    // Fade out active deck
    m_fadeOutDurationMs = fadeOutMs;
    m_fadeOutElapsedMs = 0;
    m_fadeOutStartVolume = activeDeck().output->volume();
    m_fadeOutTimer->start();
}

void AmbientPlayer::crossfadeTo(const QString& filePath, int durationMs)
{
    if (filePath.isEmpty()) {
        stop(durationMs);
        return;
    }

    if (filePath == m_currentTrack && isPlaying()) {
        return;  // Already playing this track
    }

    // Stop any in-progress crossfade
    m_crossfadeTimer->stop();
    m_fadeOutTimer->stop();

    // Start new track on inactive deck
    Deck& newDeck = inactiveDeck();
    newDeck.player->setSource(QUrl::fromLocalFile(filePath));
    newDeck.output->setVolume(0.0);
    newDeck.player->play();

    // Begin crossfade
    m_isCrossfading = true;
    m_crossfadeDurationMs = qMax(durationMs, 100);
    m_crossfadeElapsedMs = 0;
    m_crossfadeTimer->start();

    m_currentTrack = filePath;
    emit trackChanged(filePath);

    DebugConsole::info(QString("Ambient: crossfading to %1 (%2ms)")
        .arg(QFileInfo(filePath).fileName()).arg(durationMs), "Audio");
}

void AmbientPlayer::setVolume(qreal volume)
{
    m_targetVolume = qBound(0.0, volume, 1.0);
    if (!m_isCrossfading) {
        activeDeck().output->setVolume(m_targetVolume);
    }
    emit volumeChanged(m_targetVolume);
}

bool AmbientPlayer::isPlaying() const
{
    return m_decks[0].player->playbackState() == QMediaPlayer::PlayingState ||
           m_decks[1].player->playbackState() == QMediaPlayer::PlayingState;
}

void AmbientPlayer::onCrossfadeTick()
{
    m_crossfadeElapsedMs += CROSSFADE_TICK_MS;
    qreal progress = qMin(1.0, static_cast<qreal>(m_crossfadeElapsedMs) / m_crossfadeDurationMs);

    // Ease: smooth step
    qreal t = progress * progress * (3.0 - 2.0 * progress);

    // Fade in new deck, fade out old deck
    inactiveDeck().output->setVolume(m_targetVolume * t);
    activeDeck().output->setVolume(m_targetVolume * (1.0 - t));

    if (progress >= 1.0) {
        m_crossfadeTimer->stop();
        m_isCrossfading = false;

        // Stop old deck completely
        activeDeck().player->stop();
        activeDeck().output->setVolume(0.0);

        // Swap decks
        m_activeDeckIndex = 1 - m_activeDeckIndex;

        emit playbackStateChanged(true);
    }
}

void AmbientPlayer::onFadeOutFinished()
{
    m_fadeOutElapsedMs += CROSSFADE_TICK_MS;
    qreal progress = qMin(1.0, static_cast<qreal>(m_fadeOutElapsedMs) / m_fadeOutDurationMs);

    activeDeck().output->setVolume(m_fadeOutStartVolume * (1.0 - progress));

    if (progress >= 1.0) {
        m_fadeOutTimer->stop();
        activeDeck().player->stop();
        activeDeck().output->setVolume(0.0);
        m_currentTrack.clear();
        emit playbackStateChanged(false);
        emit trackChanged(QString());
        DebugConsole::info("Ambient: stopped", "Audio");
    }
}
