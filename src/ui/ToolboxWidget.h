#ifndef TOOLBOXWIDGET_H
#define TOOLBOXWIDGET_H

#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QWidget>
#include <QToolButton>
#include <QSlider>
#include <QLabel>
#include <QButtonGroup>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QAccessible>
#include "utils/FogToolMode.h"

class QAction;

class ToolSection : public QWidget
{
    Q_OBJECT

public:
    explicit ToolSection(const QString& title, QWidget* parent = nullptr);

    void addWidget(QWidget* widget);
    void addAction(QAction* action);
    void setCollapsible(bool collapsible);
    void setExpanded(bool expanded);
    bool isExpanded() const { return m_expanded; }

signals:
    void expandedChanged(bool expanded);

private slots:
    void toggleExpanded();
    void updateCollapseAnimation();

private:
    void setupUI();
    void updateExpandButton();

    QString m_title;
    bool m_collapsible;
    bool m_expanded;

    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;
    QWidget* m_headerWidget;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    QToolButton* m_expandButton;
    QLabel* m_titleLabel;
    QPropertyAnimation* m_collapseAnimation;

    static const int ANIMATION_DURATION = 200;
};

class ToolboxWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit ToolboxWidget(QWidget* parent = nullptr);
    ~ToolboxWidget();

    void setMapDisplay(class MapDisplay* mapDisplay);
    void setPlayerWindow(class PlayerWindow* playerWindow);

    FogToolMode getFogToolMode() const { return m_fogToolMode; }

    static const int BASE_WIDTH = 320;  // Base width for standard DPI
    static const int EXPANDED_WIDTH = 380;   // Extra room for expanded controls

    // DPI-aware width calculation
    static int getScaledWidth();
    static qreal getDpiScale();

public slots:
    void updateGridStatus(bool enabled);
    void updateFogStatus(bool enabled);
    void updatePlayerViewStatus(bool enabled);
    void updateZoomStatus(const QString& zoomText);
    void updateGridSize(int size);
    void updateFogBrushSize(int size);
    void updateGMOpacity(int opacity);
    void updateUndoRedoButtons(bool canUndo, bool canRedo);
    void setFogToolMode(FogToolMode mode);
    void updateFogToolButtonStates(FogToolMode mode);

signals:
    void loadMapRequested();
    void togglePlayerWindowRequested();
    void toggleGridRequested();
    void toggleGridTypeRequested();
    void openGridCalibrationRequested();
    void toggleFogOfWarRequested();
    void resetFogOfWarRequested();
    void fitToScreenRequested();
    void zoomInRequested();
    void zoomOutRequested();
    void zoomPresetRequested(qreal value);
    void fogToolModeChanged(FogToolMode mode);
    void togglePlayerViewModeRequested();
    void undoFogChangeRequested();
    void redoFogChangeRequested();
    void gridSizeChanged(int size);
    void fogBrushSizeChanged(int size);
    void gmOpacityChanged(int opacity);


protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onExpandedStateChanged();
    void updateToolboxWidth();
    void animateHover(bool entering);
    void onFogToolButtonClicked(int id);
    // S, M, L brush presets removed

private:
    void setupUI();
    // Quick access bar removed
    void createMapSection();
    void createGridSection();
    void createFogSection();
    // History section removed - Undo/Redo moved to Edit menu only
    void createViewSection();
    void applyDarkGlassTheme();
    QToolButton* createToolButton(const QString& text, const QString& tooltip, const QString& accessibleName = QString());
    QToolButton* createIconButton(const QString& icon, const QString& tooltip, const QString& accessibleName = QString());
    void connectSignals();
    void updateFogToolButtons();
    QString getFogToolModeText(FogToolMode mode) const;
    void setupAccessibility();
    void announceValueChange(const QString& announcement);
    void updateFocusOrder();

    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_mainLayout;

    // Quick Access Bar removed - duplicate functionality

    ToolSection* m_mapSection;
    ToolSection* m_gridSection;
    ToolSection* m_fogSection;
    // History section removed - using Edit menu for Undo/Redo
    ToolSection* m_viewSection;

    QToolButton* m_loadMapButton;
    QToolButton* m_playerWindowButton;

    QToolButton* m_gridToggleButton;
    QToolButton* m_gridTypeButton;
    QToolButton* m_gridCalibrationButton;
    QSlider* m_gridSizeSlider;
    QLabel* m_gridSizeLabel;

    // Fog control - SIMPLIFIED
    QToolButton* m_fogToggleButton;
    QToolButton* m_revealRectangleButton;
    QToolButton* m_playerViewButton;
    QToolButton* m_resetFogButton;
    QSlider* m_fogBrushSlider;
    QLabel* m_fogBrushLabel;
    // S, M, L brush buttons removed
    QSlider* m_gmOpacitySlider;
    QLabel* m_gmOpacityLabel;


    // Undo/Redo buttons removed - using Edit menu instead

    QToolButton* m_fitScreenButton;
    QToolButton* m_zoomInButton;
    QToolButton* m_zoomOutButton;
    QToolButton* m_zoom50Button;
    QToolButton* m_zoom100Button;
    QToolButton* m_zoom150Button;
    QLabel* m_zoomLabel;

    FogToolMode m_fogToolMode;
    bool m_expanded;
    bool m_hovered;

    QPropertyAnimation* m_widthAnimation;
    QPropertyAnimation* m_hoverAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    QTimer* m_hoverTimer;

    MapDisplay* m_mapDisplay;
    PlayerWindow* m_playerWindow;

    static const int HOVER_DELAY_MS = 300;
    static const int ANIMATION_DURATION = 200;

    // Accessibility
    QLabel* m_liveRegion;
    bool m_highContrastMode;
    bool m_reducedMotion;
};

#endif // TOOLBOXWIDGET_H