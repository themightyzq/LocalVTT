#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include <QMainWindow>

class MapDisplay;
class QPropertyAnimation;
class QWidget;
class QLabel;
class QTimer;

class PlayerWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void privacyModeChanged(bool active, const QString& modeText = QString());

public:
    explicit PlayerWindow(MapDisplay* sharedDisplay, QWidget *parent = nullptr);
    ~PlayerWindow();

    void updateDisplay();
    void forceRefresh();  // Force immediate scene refresh
    void setEnhancedStyling();
    void syncZoom(qreal zoomLevel, const QPointF& centerPoint = QPointF());
    void fitToView();
    void updateToolCursor();

    // DM-initiated view sync - pushes DM's current view to player window
    void syncViewFromDM(qreal zoomLevel, const QPointF& centerPoint);
    void resetToAutoFit();  // Return to auto-fit mode
    bool isAutoFitEnabled() const { return m_autoFitEnabled; }
    MapDisplay* getMapDisplay() const { return m_playerView; }

    // Rotation control (preserved across fitInView calls)
    void setRotation(int rotation);
    int getRotation() const { return m_rotation; }

    // Multi-monitor support
    void moveToScreen(QScreen* screen, bool fullscreen = true);
    void moveToSecondaryDisplay();  // Auto-detect and move to secondary
    static QScreen* findSecondaryScreen();  // Find first non-primary screen
    static QList<QScreen*> getAvailableScreens();

    // Privacy Shield features
    void activateBlackout();
    void activateIntermission();
    void deactivatePrivacyMode();
    bool isPrivacyModeActive() const { return m_privacyModeActive; }

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;  // DM context menu (display, privacy, view)

    // Geometry persistence
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    // Monitor change detection
    bool event(QEvent *event) override;

    // Keyboard handling
    void keyPressEvent(QKeyEvent *event) override;

    // Mouse handling for triple-click detection
    void mousePressEvent(QMouseEvent *event) override;

private:
    void animateWindowTransition(bool showing);
    void checkForMonitorChange();
    void autoFitToScreen();

    // Privacy overlay management
    void createPrivacyOverlays();
    void showBlackoutOverlay();
    void showIntermissionOverlay();
    void hidePrivacyOverlays();
    void animatePrivacyTransition(QWidget* overlay, bool show);

    // Triple-click detection
    void onTripleClickDetected();

    MapDisplay* m_sharedDisplay;
    MapDisplay* m_playerView;
    QPropertyAnimation* m_windowAnimation;
    QScreen* m_currentScreen;
    bool m_autoFitEnabled;
    int m_rotation;  // Current rotation (0, 90, 180, 270) - preserved across fitInView

    // Privacy Shield components
    QWidget* m_blackoutOverlay;
    QWidget* m_intermissionOverlay;
    QLabel* m_intermissionLabel;
    bool m_privacyModeActive;

    // Triple-click detection
    QTimer* m_tripleClickTimer;
    int m_clickCount;
    static constexpr int TRIPLE_CLICK_TIMEOUT = 500; // ms
};

#endif // PLAYERWINDOW_H