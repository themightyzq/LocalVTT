#ifndef ZOOMINDICATOR_H
#define ZOOMINDICATOR_H

#include <QWidget>
#include <QTimer>
#include <QPropertyAnimation>

class ZoomIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit ZoomIndicator(QWidget *parent = nullptr);
    ~ZoomIndicator();

    // Show zoom percentage with fade-in/out
    void showZoom(qreal zoomFactor);
    
    // Get/Set opacity for animation
    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

    // Update position relative to parent
    void updatePosition();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void hideIndicator();

private:
    qreal m_zoomFactor;
    qreal m_opacity;
    QTimer* m_hideTimer;
    QPropertyAnimation* m_fadeAnimation;
    
    // Visual constants
    static constexpr int INDICATOR_WIDTH = 90;
    static constexpr int INDICATOR_HEIGHT = 30;
    static constexpr int CORNER_MARGIN = 10;
    static constexpr int FADE_DURATION = 200;
    static constexpr int DISPLAY_DURATION = 2000;
};

#endif // ZOOMINDICATOR_H