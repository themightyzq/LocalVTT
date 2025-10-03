#include "ui/ToolboxWidget.h"
#include "utils/AnimationHelper.h"
#include <QAction>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QToolButton>
#include <QSlider>
#include <QLabel>
#include <QButtonGroup>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QPaintEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QStyle>
#include <QApplication>
#include <QKeyEvent>
#include <QAccessibleWidget>
#include <QSettings>
#include <QScreen>
#include <QGuiApplication>

ToolSection::ToolSection(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_collapsible(true)
    , m_expanded(true)
    , m_collapseAnimation(nullptr)
{
    setupUI();
    setExpanded(true);
}

void ToolSection::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_headerWidget = new QWidget(this);
    m_headerLayout = new QHBoxLayout(m_headerWidget);
    m_headerLayout->setContentsMargins(0, 12, 0, 12);
    m_headerLayout->setSpacing(12);

    m_expandButton = new QToolButton(this);
    m_expandButton->setText(">");
    m_expandButton->setFixedSize(20, 20);
    m_expandButton->setStyleSheet(
        "QToolButton {"
        "   background: transparent;"
        "   border: none;"
        "   color: rgba(255, 255, 255, 0.5);"
        "   font-size: 12px;"
        "   font-weight: normal;"
        "}"
        "QToolButton:hover {"
        "   color: rgba(255, 255, 255, 0.8);"
        "}"
    );
    connect(m_expandButton, &QToolButton::clicked, this, &ToolSection::toggleExpanded);

    m_titleLabel = new QLabel(m_title.toUpper(), this);  // Apply uppercase directly
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "   color: rgba(255, 255, 255, 0.5);"
        "   font-weight: 600;"
        "   font-size: 12px;"
        "   letter-spacing: 1px;"
        "   background: transparent;"
        "}"
    );

    m_headerLayout->addWidget(m_expandButton);
    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();

    m_headerWidget->setStyleSheet(
        "QWidget {"
        "   background: transparent;"
        "   border-bottom: 1px solid rgba(255, 255, 255, 0.1);"
        "}"
    );

    m_contentWidget = new QWidget(this);
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    qreal scale = ToolboxWidget::getDpiScale();
    int vMargin = static_cast<int>(8 * scale);  // DPI-aware vertical margins
    int spacing = static_cast<int>(4 * scale);   // DPI-aware spacing
    m_contentLayout->setContentsMargins(0, vMargin, 0, vMargin);
    m_contentLayout->setSpacing(spacing);

    m_mainLayout->addWidget(m_headerWidget);
    m_mainLayout->addWidget(m_contentWidget);

    m_collapseAnimation = new QPropertyAnimation(m_contentWidget, "maximumHeight", this);
    m_collapseAnimation->setDuration(280);
    m_collapseAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_collapseAnimation, &QPropertyAnimation::finished, this, &ToolSection::updateCollapseAnimation);
}

void ToolSection::addWidget(QWidget* widget)
{
    m_contentLayout->addWidget(widget);
}

void ToolSection::addAction(QAction* action)
{
    QToolButton* button = new QToolButton(this);
    button->setDefaultAction(action);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setMinimumHeight(32);
    button->setStyleSheet(
        "QToolButton {"
        "   background: rgba(255, 255, 255, 0.05);"
        "   border: 1px solid rgba(255, 255, 255, 0.1);"
        "   border-radius: 4px;"
        "   padding: 8px 12px;"
        "   color: #e0e0e0;"
        "   text-align: left;"
        "   font-size: 12px;"
        "}"
        "QToolButton:hover {"
        "   background: rgba(74, 158, 255, 0.2);"
        "   border-color: rgba(74, 158, 255, 0.4);"
        "   padding-left: 10px;"  // Simulate translateX with padding
        "}"
        "QToolButton:pressed {"
        "   background: rgba(74, 158, 255, 0.3);"
        "   padding: 7px 11px;"  // Simulate scale with padding reduction
        "}"
        "QToolButton:checked {"
        "   background: rgba(74, 158, 255, 0.3);"
        "   border-color: rgba(74, 158, 255, 0.6);"
        "}"
    );

    // Add premium hover animations
    AnimationHelper::setupToolButtonAnimation(button);

    addWidget(button);
}

void ToolSection::setCollapsible(bool collapsible)
{
    m_collapsible = collapsible;
    m_expandButton->setVisible(collapsible);
}

void ToolSection::setExpanded(bool expanded)
{
    if (m_expanded == expanded || !m_collapsible) return;

    m_expanded = expanded;
    updateExpandButton();

    if (m_collapseAnimation->state() == QPropertyAnimation::Running) {
        m_collapseAnimation->stop();
    }

    int startHeight = m_contentWidget->height();
    int endHeight = expanded ? m_contentWidget->sizeHint().height() : 0;

    m_collapseAnimation->setStartValue(startHeight);
    m_collapseAnimation->setEndValue(endHeight);
    m_collapseAnimation->start();

    emit expandedChanged(expanded);
}

void ToolSection::toggleExpanded()
{
    setExpanded(!m_expanded);
}

void ToolSection::updateExpandButton()
{
    m_expandButton->setText(m_expanded ? "v" : ">");
    // Removed invalid transform CSS - text change is sufficient
}

void ToolSection::updateCollapseAnimation()
{
    if (!m_expanded) {
        m_contentWidget->setMaximumHeight(0);
    } else {
        m_contentWidget->setMaximumHeight(QWIDGETSIZE_MAX);
    }
}

ToolboxWidget::ToolboxWidget(QWidget* parent)
    : QDockWidget("Tools", parent)
    , m_fogToolMode(FogToolMode::UnifiedFog)
    , m_expanded(false)
    , m_hovered(false)
    , m_mapDisplay(nullptr)
    , m_playerWindow(nullptr)
    , m_liveRegion(nullptr)
    , m_highContrastMode(false)
    , m_reducedMotion(false)
{
    // Check system accessibility preferences
    QSettings settings;
    m_highContrastMode = settings.value("accessibility/highContrast", false).toBool();
    m_reducedMotion = settings.value("accessibility/reducedMotion", false).toBool();

    setupUI();
    applyDarkGlassTheme();
    connectSignals();
    setupAccessibility();

    // DPI-aware width calculation for better readability on all displays
    int scaledWidth = getScaledWidth();
    setMinimumWidth(scaledWidth);
    setMaximumWidth(scaledWidth);
    setFixedWidth(scaledWidth);  // Fixed width - no resizing
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable);

    m_widthAnimation = new QPropertyAnimation(this, "geometry", this);
    m_widthAnimation->setDuration(m_reducedMotion ? 0 : ANIMATION_DURATION);
    m_widthAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    m_hoverTimer = new QTimer(this);
    m_hoverTimer->setSingleShot(true);
    m_hoverTimer->setInterval(HOVER_DELAY_MS);
    connect(m_hoverTimer, &QTimer::timeout, this, &ToolboxWidget::updateToolboxWidth);

    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.9);

    m_hoverAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_hoverAnimation->setDuration(m_reducedMotion ? 0 : ANIMATION_DURATION);
}

ToolboxWidget::~ToolboxWidget()
{
}

// Static method for DPI-aware scaling
qreal ToolboxWidget::getDpiScale()
{
    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    if (!primaryScreen) return 1.0;

    // Get logical DPI (typically 96 on standard displays)
    qreal logicalDpi = primaryScreen->logicalDotsPerInch();

    // Calculate scale factor (96 DPI is considered 1.0 scale)
    // Retina displays typically report 144-192 DPI
    qreal scale = logicalDpi / 96.0;

    // Clamp to reasonable values
    return qBound(1.0, scale, 2.5);
}

int ToolboxWidget::getScaledWidth()
{
    qreal scale = getDpiScale();
    return static_cast<int>(BASE_WIDTH * scale);
}

void ToolboxWidget::setupUI()
{
    // Create live region for screen reader announcements
    m_liveRegion = new QLabel(this);
    m_liveRegion->hide();
    m_liveRegion->setAccessibleDescription("Status announcements");

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setAccessibleName("Tools panel scroll area");

    m_contentWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(m_contentWidget);

    // DPI-aware margins and spacing
    qreal scale = ToolboxWidget::getDpiScale();
    int margin = static_cast<int>(16 * scale);  // Scale margins for DPI
    int spacing = static_cast<int>(8 * scale);  // Scale spacing for DPI
    m_mainLayout->setContentsMargins(margin, margin, margin, margin);
    m_mainLayout->setSpacing(spacing);

    // Quick access bar removed - duplicate functionality

    // Minimal toolbox with only fog controls
    createFogSection();     // Only fog controls shown

    m_mainLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    setWidget(m_scrollArea);
}

// Quick access bar removed - duplicate functionality

// Map section removed - Load Map in toolbar only

// Grid section removed - Grid toggle in toolbar only

void ToolboxWidget::createFogSection()
{
    // Simplified fog panel - controls only
    m_fogSection = new ToolSection("Fog Controls", this);
    m_fogSection->setCollapsible(false);  // Always visible since it's the only section

    // Create fog toggle button first
    m_fogToggleButton = createToolButton("Toggle Fog", "Toggle Fog of War (F)");
    m_fogToggleButton->setCheckable(true);
    connect(m_fogToggleButton, &QToolButton::clicked, this, &ToolboxWidget::toggleFogOfWarRequested);

    // Create fog tool group with visual separation - SIMPLIFIED to only Reveal Rect
    QWidget* fogToolsWidget = new QWidget();
    fogToolsWidget->setStyleSheet(
        "QWidget {"
        "   background: transparent;"
        "   padding: 8px 0;"
        "}"
    );
    QVBoxLayout* fogToolsLayout = new QVBoxLayout(fogToolsWidget);
    fogToolsLayout->setContentsMargins(10, 10, 10, 10);
    fogToolsLayout->setSpacing(8);

    // UNIFIED: Single fog tool with modifier-based behavior
    m_revealRectangleButton = createToolButton("Reveal Area", "Unified fog tool:\n• Click/drag: reveal areas\n• Alt + click/drag: hide areas\n• Shift + click/drag: rectangle mode\n• Double-click: clear visible area");
    m_revealRectangleButton->setCheckable(true);
    m_revealRectangleButton->setChecked(true);

    // Apply special styling to fog tool button
    QString fogToolStyle =
        "QToolButton {"
        "   background: rgba(255, 255, 255, 0.05);"
        "   border: 1px solid rgba(255, 255, 255, 0.1);"
        "   border-radius: 4px;"
        "   color: rgba(255, 255, 255, 0.6);"
        "   font-size: 14px;"
        "   font-weight: 600;"
        "}"
        "QToolButton:hover {"
        "   background: rgba(255, 255, 255, 0.1);"
        "   border: 1px solid rgba(255, 255, 255, 0.2);"
        "   color: rgba(255, 255, 255, 0.9);"
        "}"
        "QToolButton:checked {"
        "   background: rgba(74, 158, 255, 0.2);"
        "   border: 1px solid #4a9eff;"
        "   color: #4a9eff;"
        "}";

    m_revealRectangleButton->setStyleSheet(fogToolStyle);
    connect(m_revealRectangleButton, &QToolButton::clicked, this, [this]() {
        emit fogToolModeChanged(FogToolMode::UnifiedFog);
    });

    // Add unified fog tool button
    fogToolsLayout->addWidget(m_revealRectangleButton);

    // (Duplicate connection removed - handled above)

    // Brush size control with premium styling
    QWidget* brushWidget = new QWidget();
    brushWidget->setStyleSheet(
        "QWidget {"
        "   background: transparent;"
        "   padding: 8px 0;"
        "}"
    );
    QVBoxLayout* brushLayout = new QVBoxLayout(brushWidget);
    brushLayout->setContentsMargins(8, 6, 8, 6);
    brushLayout->setSpacing(4);

    m_fogBrushLabel = new QLabel("Brush Size: 200px", this);
    m_fogBrushLabel->setStyleSheet(
        "QLabel {"
        "   color: rgba(255, 255, 255, 0.8);"
        "   font-size: 11px;"
        "   font-weight: 500;"
        "   letter-spacing: 0.3px;"
        "}"
    );

    m_fogBrushSlider = new QSlider(Qt::Horizontal, this);
    m_fogBrushSlider->setMinimum(10);
    m_fogBrushSlider->setMaximum(400);
    m_fogBrushSlider->setValue(200);
    m_fogBrushSlider->setToolTip("Adjust fog brush size (10-400 pixels)");
    m_fogBrushSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "   background: rgba(255, 255, 255, 0.1);"
        "   height: 4px;"
        "   border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: #ff8e4a;"
        "   width: 12px;"
        "   height: 12px;"
        "   border-radius: 6px;"
        "   margin: -4px 0;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "   background: #ff9e6a;"
        "}"
    );
    connect(m_fogBrushSlider, &QSlider::valueChanged, this, &ToolboxWidget::fogBrushSizeChanged);
    connect(m_fogBrushSlider, &QSlider::valueChanged, this, &ToolboxWidget::updateFogBrushSize);

    // S, M, L preset buttons removed for cleaner UI
    brushLayout->addWidget(m_fogBrushLabel);
    brushLayout->addWidget(m_fogBrushSlider);

    QWidget* opacityWidget = new QWidget();
    QVBoxLayout* opacityLayout = new QVBoxLayout(opacityWidget);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacityLayout->setSpacing(2);

    m_gmOpacityLabel = new QLabel("GM Opacity: 50%", this);
    m_gmOpacityLabel->setStyleSheet("QLabel { color: #b0b0b0; font-size: 12px; padding: 2px; }");
    m_gmOpacityLabel->setMinimumWidth(120);

    m_gmOpacitySlider = new QSlider(Qt::Horizontal, this);
    m_gmOpacitySlider->setMinimum(0);
    m_gmOpacitySlider->setMaximum(100);
    m_gmOpacitySlider->setValue(50);
    m_gmOpacitySlider->setToolTip("Adjust GM fog opacity (0-100%)");
    connect(m_gmOpacitySlider, &QSlider::valueChanged, this, &ToolboxWidget::gmOpacityChanged);
    connect(m_gmOpacitySlider, &QSlider::valueChanged, this, &ToolboxWidget::updateGMOpacity);

    opacityLayout->addWidget(m_gmOpacityLabel);
    opacityLayout->addWidget(m_gmOpacitySlider);

    m_playerViewButton = createToolButton("Player View Mode", "Player View Mode (Ctrl+P)");
    m_playerViewButton->setCheckable(true);
    connect(m_playerViewButton, &QToolButton::clicked, this, &ToolboxWidget::togglePlayerViewModeRequested);

    m_resetFogButton = createToolButton("Reset Fog", "Reset Fog (Ctrl+Shift+R)");
    connect(m_resetFogButton, &QToolButton::clicked, this, &ToolboxWidget::resetFogOfWarRequested);

    m_fogSection->addWidget(m_fogToggleButton);
    m_fogSection->addWidget(fogToolsWidget);
    m_fogSection->addWidget(brushWidget);
    m_fogSection->addWidget(opacityWidget);
    m_fogSection->addWidget(m_playerViewButton);
    m_fogSection->addWidget(m_resetFogButton);

    m_mainLayout->addWidget(m_fogSection);
}

// History section removed - Undo/Redo functionality moved to Edit menu

// View section removed - Zoom controls via keyboard only

QToolButton* ToolboxWidget::createToolButton(const QString& text, const QString& tooltip, const QString& accessibleName)
{
    QToolButton* button = new QToolButton(this);
    button->setText(text);
    button->setToolTip(tooltip);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);

    // DPI-aware button sizing
    qreal scale = ToolboxWidget::getDpiScale();
    int buttonHeight = static_cast<int>(40 * scale);  // Scale button height for DPI
    button->setMinimumHeight(buttonHeight);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);  // Expand to fill width

    // Scale font size for DPI
    QFont font = button->font();
    font.setPointSize(static_cast<int>(13 * scale));
    button->setFont(font);

    // Set accessible name and description
    if (!accessibleName.isEmpty()) {
        button->setAccessibleName(accessibleName);
    } else {
        button->setAccessibleName(text);
    }
    button->setAccessibleDescription(tooltip);

    // Ensure button can receive focus
    button->setFocusPolicy(Qt::StrongFocus);

    return button;
}

QToolButton* ToolboxWidget::createIconButton(const QString& icon, const QString& tooltip, const QString& accessibleName)
{
    QToolButton* button = new QToolButton(this);
    button->setText(icon);
    button->setToolTip(tooltip);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setMinimumHeight(32);

    // Set accessible name and description
    if (!accessibleName.isEmpty()) {
        button->setAccessibleName(accessibleName);
    } else {
        button->setAccessibleName(tooltip);
    }
    button->setAccessibleDescription(tooltip);

    button->setFocusPolicy(Qt::StrongFocus);

    return button;
}

void ToolboxWidget::applyDarkGlassTheme()
{
    QString toolboxStyle =
        "ToolboxWidget {"
        "   background: #1a1a1a;"
        "   border-right: 1px solid rgba(255, 255, 255, 0.1);"
        "}"
        "QScrollArea {"
        "   background: transparent;"
        "   border: none;"
        "}"
        "QScrollBar:vertical {"
        "   background: transparent;"
        "   width: 8px;"
        "   margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: rgba(255, 255, 255, 0.2);"
        "   border-radius: 4px;"
        "   min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background: rgba(255, 255, 255, 0.3);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   background: none;"
        "   height: 0px;"
        "}"
        "QToolButton {"
        "   background: transparent;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 12px 20px;"
        "   color: rgba(255, 255, 255, 0.7);"
        "   font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;"
        "   font-size: 14px;"
        "   font-weight: 500;"
        "   text-align: left;"
        "   min-height: 40px;"
        "}"
        "QToolButton:hover {"
        "   background: rgba(255, 255, 255, 0.08);"
        "   color: rgba(255, 255, 255, 0.95);"
        "}"
        "QToolButton:pressed {"
        "   background: rgba(255, 255, 255, 0.12);"
        "}"
        "QToolButton:checked {"
        "   background: rgba(74, 158, 255, 0.2);"
        "   border: 1px solid #4a9eff;"
        "   color: #4a9eff;"
        "   font-weight: 600;"
        "}"
        "QLabel {"
        "   background: transparent;"
        "   color: rgba(255, 255, 255, 0.85);"
        "}";

    setStyleSheet(toolboxStyle);
}

void ToolboxWidget::connectSignals()
{
    // Only fog section exists now
    if (m_fogSection) {
        connect(m_fogSection, &ToolSection::expandedChanged, this, &ToolboxWidget::onExpandedStateChanged);
    }
}

void ToolboxWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Simple flat background
    painter.fillRect(rect(), QColor(26, 26, 26));

    // Subtle right border
    painter.setPen(QColor(255, 255, 255, 25));
    painter.drawLine(width() - 1, 0, width() - 1, height());

    QDockWidget::paintEvent(event);
}

void ToolboxWidget::keyPressEvent(QKeyEvent* event)
{
    // Keyboard navigation shortcuts
    if (event->key() == Qt::Key_F6) {
        // Navigate to fog section
        if (m_revealRectangleButton) {
            m_revealRectangleButton->setFocus();
        }
    } else if (event->key() == Qt::Key_Escape) {
        // Collapse fog section
        if (m_fogSection) {
            m_fogSection->setExpanded(false);
        }
        updateToolboxWidth();
    } else if (event->modifiers() & Qt::AltModifier) {
        // Alt+1,2,3 for brush size presets - removed
        // Direct slider value setting instead
        if (event->key() == Qt::Key_1) {
            m_fogBrushSlider->setValue(25);
        } else if (event->key() == Qt::Key_2) {
            m_fogBrushSlider->setValue(50);
        } else if (event->key() == Qt::Key_3) {
            m_fogBrushSlider->setValue(100);
        }
    } else {
        QDockWidget::keyPressEvent(event);
    }
}

void ToolboxWidget::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    if (!m_expanded) {
        m_hoverTimer->start();
    }
    animateHover(true);
    QDockWidget::enterEvent(event);
}

void ToolboxWidget::leaveEvent(QEvent* event)
{
    m_hovered = false;
    m_hoverTimer->stop();
    if (m_expanded && !geometry().contains(QCursor::pos())) {
        updateToolboxWidth();
    }
    animateHover(false);
    QDockWidget::leaveEvent(event);
}

void ToolboxWidget::updateToolboxWidth()
{
    // Disabled - no hover width changes for cleaner UI
    // The toolbox stays at a fixed width for consistency
    return;

    /*
    bool shouldExpand = m_hovered && !m_expanded;
    bool shouldCollapse = !m_hovered && m_expanded;

    if (shouldExpand || shouldCollapse) {
        m_expanded = !m_expanded;

        QRect currentGeometry = geometry();
        QRect targetGeometry = currentGeometry;
        int targetWidth = m_expanded ? EXPANDED_WIDTH : COLLAPSED_WIDTH;
        targetGeometry.setWidth(targetWidth);
        setMinimumWidth(targetWidth);
        setMaximumWidth(targetWidth);

        m_widthAnimation->setStartValue(currentGeometry);
        m_widthAnimation->setEndValue(targetGeometry);
        m_widthAnimation->start();
    }
    */
}

void ToolboxWidget::animateHover(bool entering)
{
    if (m_hoverAnimation->state() == QPropertyAnimation::Running) {
        m_hoverAnimation->stop();
    }

    qreal targetOpacity = entering ? 1.0 : 0.9;
    m_hoverAnimation->setStartValue(m_opacityEffect->opacity());
    m_hoverAnimation->setEndValue(targetOpacity);
    m_hoverAnimation->start();
}

void ToolboxWidget::onExpandedStateChanged()
{
}

void ToolboxWidget::onFogToolButtonClicked(int id)
{
    setFogToolMode(static_cast<FogToolMode>(id));
    emit fogToolModeChanged(m_fogToolMode);
}

// S, M, L brush button functions removed

void ToolboxWidget::setMapDisplay(MapDisplay* mapDisplay)
{
    m_mapDisplay = mapDisplay;
}

void ToolboxWidget::setPlayerWindow(PlayerWindow* playerWindow)
{
    m_playerWindow = playerWindow;
}

void ToolboxWidget::updateGridStatus(bool enabled)
{
    // Grid toggle button removed - grid control in toolbar only
    Q_UNUSED(enabled);
}

void ToolboxWidget::updateFogStatus(bool enabled)
{
    m_fogToggleButton->setChecked(enabled);
}

void ToolboxWidget::updatePlayerViewStatus(bool enabled)
{
    m_playerViewButton->setChecked(enabled);
}


void ToolboxWidget::updateZoomStatus(const QString& zoomText)
{
    // Zoom controls removed from toolbox - keyboard shortcuts only
    Q_UNUSED(zoomText);
}

void ToolboxWidget::updateGridSize(int size)
{
    // Grid size control removed from toolbox
    Q_UNUSED(size);
}

void ToolboxWidget::updateFogBrushSize(int size)
{
    m_fogBrushLabel->setText(QString("Brush Size: %1px").arg(size));
}

void ToolboxWidget::updateGMOpacity(int opacity)
{
    m_gmOpacityLabel->setText(QString("GM Opacity: %1%").arg(opacity));
}

void ToolboxWidget::updateUndoRedoButtons(bool canUndo, bool canRedo)
{
    // Undo/Redo buttons removed from toolbox - handled by Edit menu
    Q_UNUSED(canUndo);
    Q_UNUSED(canRedo);
}

void ToolboxWidget::setFogToolMode(FogToolMode mode)
{
    if (m_fogToolMode == mode) return;

    m_fogToolMode = mode;
    updateFogToolButtons();
}

void ToolboxWidget::updateFogToolButtons()
{
    // Update button checked states to reflect current tool mode
    if (m_revealRectangleButton) {
        m_revealRectangleButton->setChecked(m_fogToolMode == FogToolMode::UnifiedFog);
    }

    // Note: Only UnifiedFog button exists in current simplified UI
    // All fog operations handled through modifier keys
}

void ToolboxWidget::updateFogToolButtonStates(FogToolMode mode)
{
    // Set the internal mode (without emitting signal to avoid loops)
    m_fogToolMode = mode;

    // Update visual states of all fog tool buttons
    updateFogToolButtons();
}

QString ToolboxWidget::getFogToolModeText(FogToolMode mode) const
{
    switch (mode) {
        case FogToolMode::UnifiedFog: return "Unified Fog Tool";
        case FogToolMode::DrawPen: return "Drawing Pen";
        case FogToolMode::DrawEraser: return "Drawing Eraser";
        default: return "Unknown";
    }
}

void ToolboxWidget::setupAccessibility()
{
    // Set accessible name for the main widget
    setAccessibleName("VTT Toolbox");
    setAccessibleDescription("Control panel for map display, grid, fog of war, and view settings");

    // Enable accessibility for all interactive widgets
    QAccessible::setActive(true);

    // Update focus order for logical keyboard navigation
    updateFocusOrder();
}

void ToolboxWidget::announceValueChange(const QString& announcement)
{
    // Update live region for screen reader announcements
    if (m_liveRegion) {
        m_liveRegion->setText(announcement);
        QAccessibleEvent event(m_liveRegion, QAccessible::NameChanged);
        QAccessible::updateAccessibility(&event);
    }
}

void ToolboxWidget::updateFocusOrder()
{
    // Set logical tab order for fog controls only
    if (m_revealRectangleButton && m_fogBrushSlider) {
        setTabOrder(m_revealRectangleButton, m_fogBrushSlider);
    }
    if (m_fogBrushSlider && m_gmOpacitySlider) {
        setTabOrder(m_fogBrushSlider, m_gmOpacitySlider);
    }
    if (m_gmOpacitySlider && m_playerViewButton) {
        setTabOrder(m_gmOpacitySlider, m_playerViewButton);
    }
    if (m_playerViewButton && m_resetFogButton) {
        setTabOrder(m_playerViewButton, m_resetFogButton);
    }
}



bool ToolboxWidget::eventFilter(QObject* watched, QEvent* event)
{
    // Monitor focus changes for accessibility announcements
    if (event->type() == QEvent::FocusIn) {
        QWidget* widget = qobject_cast<QWidget*>(watched);
        if (widget) {
            // Announce widget focus to screen readers
            QString announcement = widget->accessibleName();
            if (!announcement.isEmpty()) {
                announceValueChange(QString("Focus on %1").arg(announcement));
            }
        }
    }

    return QDockWidget::eventFilter(watched, event);
}