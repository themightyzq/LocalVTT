#include "controllers/ViewZoomController.h"
#include "graphics/MapDisplay.h"
#include "ui/PlayerWindow.h"
#include "graphics/FogOfWar.h"
#include "ui/DarkTheme.h"
#include "utils/DebugConsole.h"
#include <QAction>
#include <QLabel>
#include <QToolBar>
#include <QSpinBox>

ViewZoomController::ViewZoomController(QObject *parent)
    : QObject(parent)
    , m_mapDisplay(nullptr)
    , m_playerWindow(nullptr)
    , m_fitToScreenAction(nullptr)
    , m_zoomInAction(nullptr)
    , m_zoomOutAction(nullptr)
    , m_zoomStatusLabel(nullptr)
    , m_playerViewModeEnabled(false)
{
}

void ViewZoomController::setMapDisplay(MapDisplay* display)
{
    m_mapDisplay = display;
    if (m_mapDisplay) {
        connect(m_mapDisplay, &MapDisplay::zoomChanged,
                this, &ViewZoomController::updateZoomStatus);
        connect(m_mapDisplay, &MapDisplay::zoomChanged,
                this, &ViewZoomController::syncZoomWithPlayer);
    }
}

void ViewZoomController::setPlayerWindow(PlayerWindow* playerWindow)
{
    m_playerWindow = playerWindow;
}

void ViewZoomController::setZoomActions(QAction* fitAction, QAction* zoomInAction, QAction* zoomOutAction)
{
    m_fitToScreenAction = fitAction;
    m_zoomInAction = zoomInAction;
    m_zoomOutAction = zoomOutAction;
}

void ViewZoomController::setZoomStatusLabel(QLabel* label)
{
    m_zoomStatusLabel = label;
}

qreal ViewZoomController::getCurrentZoom() const
{
    if (!m_mapDisplay) return 1.0;
    return m_mapDisplay->getZoomLevel();
}

void ViewZoomController::fitToScreen()
{
    if (!m_mapDisplay) return;

    m_mapDisplay->fitMapToView();
    updateZoomStatus();

    emit requestStatus("Map fitted to screen");
    emit zoomChanged(getCurrentZoom());

    DebugConsole::info("Map fitted to screen", "View");
}

void ViewZoomController::zoomIn()
{
    if (!m_mapDisplay) return;

    qreal currentZoom = getCurrentZoom();
    qreal newZoom = currentZoom * 1.25;

    if (newZoom > 5.0) {
        newZoom = 5.0;
        emit requestStatus("Maximum zoom reached");
    }

    m_mapDisplay->setZoomLevel(newZoom);
    updateZoomStatus();

    emit zoomChanged(newZoom);

    DebugConsole::info(
        QString("Zoomed in to %1%").arg(static_cast<int>(newZoom * 100)),
        "View"
    );
}

void ViewZoomController::zoomOut()
{
    if (!m_mapDisplay) return;

    qreal currentZoom = getCurrentZoom();
    qreal newZoom = currentZoom * 0.8;

    if (newZoom < 0.1) {
        newZoom = 0.1;
        emit requestStatus("Minimum zoom reached");
    }

    m_mapDisplay->setZoomLevel(newZoom);
    updateZoomStatus();

    emit zoomChanged(newZoom);

    DebugConsole::info(
        QString("Zoomed out to %1%").arg(static_cast<int>(newZoom * 100)),
        "View"
    );
}

void ViewZoomController::updateZoomStatus()
{
    if (!m_mapDisplay || !m_zoomStatusLabel) return;

    qreal zoom = getCurrentZoom();
    int zoomPercent = static_cast<int>(zoom * 100);
    m_zoomStatusLabel->setText(QString("Zoom: %1%").arg(zoomPercent));
}

void ViewZoomController::syncZoomWithPlayer(qreal zoomLevel)
{
    if (!m_playerWindow || !m_playerWindow->isVisible()) return;

    m_playerWindow->syncZoom(zoomLevel);

    DebugConsole::info(
        QString("Player window zoom synced to %1%").arg(static_cast<int>(zoomLevel * 100)),
        "View"
    );
}



void ViewZoomController::togglePlayerViewMode()
{
    if (!m_mapDisplay) return;

    m_playerViewModeEnabled = !m_playerViewModeEnabled;

    // Toggle player view mode - this affects how fog is displayed
    // In player view mode, DM sees what players see (full opacity fog)
    // In GM view mode, DM sees semi-transparent fog
    // Note: This feature may need to be implemented in MapDisplay/FogOfWar
    m_mapDisplay->update();

    if (m_playerViewModeEnabled) {
        emit requestStatus("Player View Mode ON - Seeing map as players see it");
        DebugConsole::info("Player view mode enabled", "View");
    } else {
        emit requestStatus("Player View Mode OFF - GM view restored");
        DebugConsole::info("Player view mode disabled", "View");
    }
}

QSpinBox* ViewZoomController::createToolbarActions(QToolBar* toolbar)
{
    // Zoom Out
    QAction* zoomOutAction = new QAction(QIcon(":/icons/zoom_out.svg"), "Zoom Out", toolbar);
    zoomOutAction->setToolTip("<b>Zoom Out</b><br><i>Shortcut: -</i>");
    zoomOutAction->setShortcut(QKeySequence("-"));
    zoomOutAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(zoomOutAction, &QAction::triggered, this, &ViewZoomController::zoomOut);
    toolbar->addAction(zoomOutAction);

    // Zoom In
    QAction* zoomInAction = new QAction(QIcon(":/icons/zoom_in.svg"), "Zoom In", toolbar);
    zoomInAction->setToolTip("<b>Zoom In</b><br><i>Shortcut: + or =</i>");
    zoomInAction->setShortcut(QKeySequence("="));
    zoomInAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(zoomInAction, &QAction::triggered, this, &ViewZoomController::zoomIn);
    toolbar->addAction(zoomInAction);

    // Hidden + shortcut (added to parent window, not toolbar)
    QWidget* parentWindow = toolbar->parentWidget();
    if (parentWindow) {
        QAction* zoomInPlus = new QAction(parentWindow);
        zoomInPlus->setShortcut(QKeySequence("+"));
        zoomInPlus->setShortcutContext(Qt::ApplicationShortcut);
        connect(zoomInPlus, &QAction::triggered, this, &ViewZoomController::zoomIn);
        parentWindow->addAction(zoomInPlus);
    }

    // Fit to View
    QAction* zoomFitAction = new QAction(QIcon(":/icons/zoom_fit.svg"), "Fit", toolbar);
    zoomFitAction->setToolTip("<b>Fit to View</b><br>Fit entire map to window<br><i>Shortcut: /</i>");
    zoomFitAction->setShortcut(QKeySequence("/"));
    zoomFitAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(zoomFitAction, &QAction::triggered, this, &ViewZoomController::fitToScreen);
    toolbar->addAction(zoomFitAction);

    // Store actions for setZoomActions
    setZoomActions(zoomFitAction, zoomInAction, zoomOutAction);

    // Zoom spinner
    QLabel* zoomLabel = new QLabel("Zoom:", toolbar);
    zoomLabel->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: 500; padding: 0 4px;")
        .arg(DarkTheme::hex(DarkTheme::TextPrimary))
        .arg(DarkTheme::FontBase));
    toolbar->addWidget(zoomLabel);

    QSpinBox* zoomSpinner = new QSpinBox(toolbar);
    zoomSpinner->setMinimum(10);
    zoomSpinner->setMaximum(500);
    zoomSpinner->setValue(100);
    zoomSpinner->setSuffix("%");
    zoomSpinner->setSingleStep(5);
    zoomSpinner->setFixedWidth(85);
    zoomSpinner->setToolTip("<b>Zoom Level</b><br>Adjust map zoom percentage<br>Range: 10-500%");
    zoomSpinner->setStyleSheet(DarkTheme::spinBoxStyle());
    toolbar->addWidget(zoomSpinner);

    return zoomSpinner;
}