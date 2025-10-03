#ifndef LOADINGOVERLAY_H
#define LOADINGOVERLAY_H

#include <QWidget>
#include <QElapsedTimer>
#include <QString>

class QLabel;
class QProgressBar;
class QPropertyAnimation;
class QGraphicsOpacityEffect;
class QParallelAnimationGroup;
class QTimer;

class LoadingOverlay : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(int pulseGlow READ pulseGlow WRITE setPulseGlow)

public:
    explicit LoadingOverlay(QWidget* parent = nullptr);
    ~LoadingOverlay();

    void showLoading(const QString& message = QString(),
                     bool indeterminate = true,
                     int progressMax = 100);
    void updateProgress(int value, const QString& subMessage = QString());
    void updateMessage(const QString& message);
    void hideLoading();

    bool isLoading() const { return m_isLoading; }
    qreal opacity() const { return m_currentOpacity; }
    void setOpacity(qreal opacity);

    int pulseGlow() const { return m_pulseGlowValue; }
    void setPulseGlow(int value);

signals:
    void cancelled();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void updateElapsedTime();
    void startPulseAnimation();
    void onFadeInFinished();
    void onFadeOutFinished();

private:
    void setupUI();
    void centerOnParent();
    void applyProfessionalStyling();
    void startAnimations();
    void stopAnimations();

    QLabel* m_messageLabel;
    QLabel* m_subMessageLabel;
    QLabel* m_elapsedTimeLabel;
    QProgressBar* m_progressBar;
    QWidget* m_contentWidget;

    QPropertyAnimation* m_fadeAnimation;
    QPropertyAnimation* m_pulseAnimation;
    QParallelAnimationGroup* m_animationGroup;
    QGraphicsOpacityEffect* m_opacityEffect;
    QTimer* m_elapsedTimer;
    QElapsedTimer m_loadTimer;

    QString m_currentMessage;
    bool m_isLoading;
    bool m_isIndeterminate;
    qreal m_currentOpacity;
    int m_pulseGlowValue;

    static constexpr int CONTENT_WIDTH = 400;
    static constexpr int CONTENT_HEIGHT = 180;
    static constexpr int FADE_DURATION = 300;
    static constexpr int PULSE_DURATION = 2000;
};

#endif // LOADINGOVERLAY_H