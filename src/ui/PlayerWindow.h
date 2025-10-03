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

    void setEnhancedStyling();
    void syncZoom(qreal zoomLevel, const QPointF& centerPoint = QPointF());
    void fitToView();
    void updateToolCursor();
    MapDisplay* getMapDisplay() const { return m_playerView; }

    // Privacy Shield features
    void activateBlackout();
    void activateIntermission();
    void deactivatePrivacyMode();
    bool isPrivacyModeActive() const { return m_privacyModeActive; }

    void forceRefresh();
    void updateDisplay();

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

    // Geometry persistence
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    // Monitor change detection
    bool event(QEvent *event) override;

    // Keyboard handling
    void keyPressEvent(QKeyEvent *event) override;

    // Mouse handling for triple-click detection
    void mousePressEvent(QMouseEvent *event) override;

    // CRITICAL FIX: Handle window state changes to prevent black screen
    void changeEvent(QEvent *event) override;

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

    // Privacy Shield components
    QWidget* m_blackoutOverlay;
    QWidget* m_intermissionOverlay;
    QLabel* m_intermissionLabel;
    bool m_privacyModeActive;

    // Triple-click detection
    QTimer* m_tripleClickTimer;
    int m_clickCount;
    static constexpr int TRIPLE_CLICK_TIMEOUT = 500; // ms

    // Consolidated refresh timer (Priority 5 fix)
    QTimer* m_refreshTimer;
    QTimer* m_autoFitTimer;
    static constexpr int REFRESH_DEBOUNCE_MS = 100;
    static constexpr int AUTOFIT_DEBOUNCE_MS = 200;

    void scheduleRefresh();
    void scheduleAutoFit();
};

#endif // PLAYERWINDOW_H