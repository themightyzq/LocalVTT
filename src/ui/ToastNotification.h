#ifndef TOASTNOTIFICATION_H
#define TOASTNOTIFICATION_H

#include <QWidget>
#include <QString>
#include <QTimer>

class QLabel;
class QPropertyAnimation;
class QGraphicsDropShadowEffect;

class ToastNotification : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(int slidePosition READ slidePosition WRITE setSlidePosition)

public:
    enum class Type {
        Info,
        Success,
        Warning,
        Error
    };

    enum class Position {
        TopCenter,
        TopRight,
        BottomCenter,
        BottomRight,
        BottomLeft
    };

    explicit ToastNotification(QWidget* parent = nullptr);
    ~ToastNotification();

    void showMessage(const QString& message, Type type = Type::Info, int duration = 3000);
    void setPosition(Position pos) { m_position = pos; }

    qreal opacity() const { return m_currentOpacity; }
    void setOpacity(qreal opacity);

    int slidePosition() const { return m_slidePos; }
    void setSlidePosition(int pos);

    static ToastNotification* instance(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void startHideAnimation();
    void onAnimationFinished();

private:
    void updatePosition();
    void setupStyle(Type type);

    QLabel* m_messageLabel;
    QTimer* m_hideTimer;
    QPropertyAnimation* m_fadeInAnimation;
    QPropertyAnimation* m_fadeOutAnimation;
    QPropertyAnimation* m_slideAnimation;
    QGraphicsDropShadowEffect* m_shadowEffect;

    Position m_position;
    Type m_currentType;
    qreal m_currentOpacity;
    int m_slidePos;

    static constexpr int TOAST_WIDTH = 350;
    static constexpr int TOAST_MIN_HEIGHT = 60;
    static constexpr int TOAST_MARGIN = 20;
    static constexpr int ANIMATION_DURATION = 250;
    static constexpr int SLIDE_DISTANCE = 30;
};

#endif // TOASTNOTIFICATION_H