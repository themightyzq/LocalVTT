#ifndef LOADINGPROGRESSWIDGET_H
#define LOADINGPROGRESSWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPropertyAnimation>

class LoadingProgressWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit LoadingProgressWidget(QWidget *parent = nullptr);
    ~LoadingProgressWidget();

    // Show/hide progress with animation
    void showProgress();
    void hideProgress();

    // Update progress percentage (0-100)
    void setProgress(int percentage);
    void setLoadingText(const QString& text);

    // Get/Set opacity for animation
    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

    // Update position relative to parent
    void updatePosition();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    int m_progress;
    QString m_loadingText;
    qreal m_opacity;
    QPropertyAnimation* m_fadeAnimation;

    // Visual constants
    static constexpr int WIDGET_WIDTH = 200;
    static constexpr int WIDGET_HEIGHT = 60;
    static constexpr int PROGRESS_BAR_HEIGHT = 8;
    static constexpr int FADE_DURATION = 200;
};

#endif // LOADINGPROGRESSWIDGET_H