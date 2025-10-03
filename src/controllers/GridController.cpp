#include "controllers/GridController.h"
#include "ui/MainWindow.h"
#include "graphics/MapDisplay.h"
#include "graphics/GridOverlay.h"
// GridCalibrationDialog removed - simplified calibration
#include "utils/SettingsManager.h"
#include "utils/DebugConsole.h"
#include <QSlider>
#include <QLabel>
#include <QAction>
#include <QMessageBox>

GridController::GridController(QObject *parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_mapDisplay(nullptr)
    , m_gridSizeSlider(nullptr)
    , m_gridSizeLabel(nullptr)
    , m_toggleGridAction(nullptr)
    , m_gridEnabled(true)
{
    m_gridEnabled = SettingsManager::instance().loadGridEnabled();
}

void GridController::attachToMainWindow(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
}

void GridController::setMapDisplay(MapDisplay* display)
{
    m_mapDisplay = display;
    if (m_mapDisplay) {
        m_mapDisplay->setGridEnabled(m_gridEnabled);
    }
}

void GridController::setGridSizeSlider(QSlider* slider, QLabel* label)
{
    m_gridSizeSlider = slider;
    m_gridSizeLabel = label;

    if (m_gridSizeSlider) {
        connect(m_gridSizeSlider, &QSlider::valueChanged,
                this, &GridController::onGridSizeChanged);
        updateGridSizeSlider();
    }
}

void GridController::setToggleAction(QAction* action)
{
    m_toggleGridAction = action;
    if (m_toggleGridAction) {
        m_toggleGridAction->setChecked(m_gridEnabled);
    }
}

int GridController::getCurrentGridSize() const
{
    if (!m_mapDisplay) return 50;

    if (GridOverlay* grid = m_mapDisplay->getGridOverlay()) {
        return static_cast<int>(grid->getGridSize());
    }
    return 50;
}

void GridController::toggleGrid()
{
    m_gridEnabled = !m_gridEnabled;

    if (m_mapDisplay) {
        m_mapDisplay->setGridEnabled(m_gridEnabled);
    }

    SettingsManager::instance().saveGridEnabled(m_gridEnabled);

    if (m_toggleGridAction) {
        m_toggleGridAction->setChecked(m_gridEnabled);
    }

    emit gridToggled(m_gridEnabled);
    emit requestStatus(m_gridEnabled ? "Grid enabled" : "Grid disabled");

    DebugConsole::info(
        m_gridEnabled ? "Grid toggled ON" : "Grid toggled OFF",
        "Grid"
    );
}

void GridController::toggleGridType()
{
    // Grid type switching removed - only square grids supported now
    if (!m_mapDisplay) return;

    GridOverlay* grid = m_mapDisplay->getGridOverlay();
    if (!grid) return;

    m_mapDisplay->update();

    emit requestStatus("Grid type: Square (only type supported)");
    emit gridTypeChanged();

    DebugConsole::info("Grid type is square (only type supported)", "Grid");
}

void GridController::setStandardGrid()
{
    if (!m_mapDisplay) return;

    GridOverlay* grid = m_mapDisplay->getGridOverlay();
    if (!grid) return;

    const int STANDARD_GRID_SIZE = 50;
    grid->setGridSize(STANDARD_GRID_SIZE);
    // Grid type is always square now

    if (!m_gridEnabled) {
        toggleGrid();
    }

    updateGridSizeSlider();
    m_mapDisplay->update();

    emit requestStatus("Grid reset to D&D standard (50px squares)");
    emit gridSizeChanged(STANDARD_GRID_SIZE);
    emit gridTypeChanged();

    DebugConsole::info(
        "Grid reset to standard D&D configuration",
        "Grid"
    );
}

void GridController::showGridInfo()
{
    if (!m_mapDisplay) return;

    GridOverlay* grid = m_mapDisplay->getGridOverlay();
    if (!grid) return;

    int gridSize = static_cast<int>(grid->getGridSize());
    QString status = m_gridEnabled ? "Enabled" : "Disabled";

    QString info = QString("Grid Information:\n\n"
                          "Status: %1\n"
                          "Type: Square\n"
                          "Size: %2 pixels\n"
                          "Scale: 1 square = 5 feet")
                          .arg(status)
                          .arg(gridSize);

    QMessageBox::information(m_mainWindow, "Grid Information", info);

    DebugConsole::info(
        QString("Grid info displayed - %1, Square, %2px")
            .arg(status).arg(gridSize),
        "Grid"
    );
}

void GridController::openCalibration()
{
    if (!m_mainWindow || !m_mapDisplay) return;

    // Grid calibration dialog removed - use simple message instead
    QMessageBox::information(m_mainWindow, "Grid Calibration",
        "Grid calibration has been simplified.\n\n"
        "Use the grid size slider to adjust the grid size manually.\n"
        "The default size is optimized for standard D&D play.");

    emit requestStatus("Use grid slider to adjust size");
    DebugConsole::info("Grid calibration dialog removed - use slider instead", "GridController");
}

void GridController::onGridSizeChanged(int value)
{
    if (!m_mapDisplay) return;

    GridOverlay* grid = m_mapDisplay->getGridOverlay();
    if (!grid) return;

    grid->setGridSize(value);

    if (m_gridSizeLabel) {
        m_gridSizeLabel->setText(QString("Grid Size: %1px").arg(value));
    }

    m_mapDisplay->update();

    emit gridSizeChanged(value);

    DebugConsole::info(
        QString("Grid size changed to %1px").arg(value),
        "Grid"
    );
}

void GridController::updateGridSizeSlider()
{
    if (!m_gridSizeSlider || !m_mapDisplay) return;

    GridOverlay* grid = m_mapDisplay->getGridOverlay();
    if (!grid) return;

    int gridSize = static_cast<int>(grid->getGridSize());

    m_gridSizeSlider->blockSignals(true);
    m_gridSizeSlider->setValue(gridSize);
    m_gridSizeSlider->blockSignals(false);

    if (m_gridSizeLabel) {
        m_gridSizeLabel->setText(QString("Grid Size: %1px").arg(gridSize));
    }
}