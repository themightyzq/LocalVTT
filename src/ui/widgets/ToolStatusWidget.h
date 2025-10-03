#ifndef TOOLSTATUSWIDGET_H
#define TOOLSTATUSWIDGET_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include "utils/ToolType.h"
#include "utils/FogToolMode.h"

class QLabel;
class QHBoxLayout;
class QTimer;

class ToolStatusWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit ToolStatusWidget(QWidget* parent = nullptr);
    ~ToolStatusWidget() = default;

    void setCurrentTool(ToolType tool);
    void setFogToolMode(FogToolMode mode);
    void updateHintText(const QString& hint);

    qreal opacity() const;
    void setOpacity(qreal opacity);

public slots:
    void onToolChanged(ToolType tool);
    void onFogToolModeChanged(FogToolMode mode);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private slots:
    void onAnimationFinished();
    void onUpdateTimer();

private:
    void setupUI();
    void setupAnimations();
    void updateContent();
    void startFadeTransition();
    QString getToolIcon(ToolType tool) const;
    QString getToolName(ToolType tool) const;
    QString getToolHint(ToolType tool) const;
    QString getFogModeText(FogToolMode mode) const;

    QLabel* m_iconLabel;
    QLabel* m_nameLabel;
    QLabel* m_hintLabel;
    QHBoxLayout* m_layout;

    QPropertyAnimation* m_fadeAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    QTimer* m_updateTimer;

    ToolType m_currentTool;
    FogToolMode m_currentFogMode;
    QString m_customHint;
    bool m_animatingTransition;

    static constexpr int ANIMATION_DURATION = 180;
    static constexpr int UPDATE_DELAY = 100;
};

#endif // TOOLSTATUSWIDGET_H