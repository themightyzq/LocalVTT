#include "DebugConsoleWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QGroupBox>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QScrollBar>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

DebugConsoleWidget::DebugConsoleWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentFilter("All")
{
    setupUI();
    
    DebugConsole::instance()->setWidget(this);
    if (auto* console = DebugConsole::instance()) {
        console->setWidget(this);
        connect(console, &DebugConsole::metricsUpdated,
                this, &DebugConsoleWidget::onMetricsUpdated);
    }
    
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &DebugConsoleWidget::updateDisplayedMessages);
    m_updateTimer->start(UPDATE_INTERVAL);
    
    updateMetricsDisplay();
    updateSystemInfoDisplay();
    updateVTTDiagnostics();
}

void DebugConsoleWidget::setupUI()
{
    setWindowTitle("Debug Console");
    setMinimumSize(800, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget;
    mainLayout->addWidget(m_tabWidget);
    
    setupLogTab();
    setupMetricsTab();
    setupSystemTab();
    setupVTTTab();
}

void DebugConsoleWidget::setupLogTab()
{
    m_logTab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(m_logTab);
    
    QHBoxLayout* controlsLayout = new QHBoxLayout;
    
    QLabel* filterLabel = new QLabel("Filter:");
    m_filterCombo = new QComboBox;
    m_filterCombo->addItems({"All", "INFO", "WARN", "ERROR", "PERF", "SYS", "VTT"});
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DebugConsoleWidget::onFilterChanged);
    
    m_clearButton = new QPushButton("Clear");
    connect(m_clearButton, &QPushButton::clicked, this, &DebugConsoleWidget::onClearClicked);
    
    m_exportButton = new QPushButton("Export");
    connect(m_exportButton, &QPushButton::clicked, this, &DebugConsoleWidget::onExportClicked);
    
    controlsLayout->addWidget(filterLabel);
    controlsLayout->addWidget(m_filterCombo);
    controlsLayout->addStretch();
    controlsLayout->addWidget(m_clearButton);
    controlsLayout->addWidget(m_exportButton);
    
    m_logDisplay = new QTextEdit;
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setFont(QFont("Consolas", 10));
    
    m_logDisplay->setStyleSheet("QTextEdit { background-color: #2b2b2b; color: #ffffff; selection-background-color: #4a90e2; }");
    
    layout->addLayout(controlsLayout);
    layout->addWidget(m_logDisplay);
    
    m_tabWidget->addTab(m_logTab, "Log Output");
}

void DebugConsoleWidget::setupMetricsTab()
{
    m_metricsTab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(m_metricsTab);
    
    QGroupBox* performanceGroup = new QGroupBox("Performance Metrics");
    QVBoxLayout* perfLayout = new QVBoxLayout(performanceGroup);
    
    m_fpsLabel = new QLabel("FPS: --");
    m_memoryLabel = new QLabel("Memory Usage: --");
    m_loadTimeLabel = new QLabel("Last Load Time: --");
    m_averageLoadTimeLabel = new QLabel("Average Load Time: --");
    m_totalLoadsLabel = new QLabel("Total Loads: 0");
    
    perfLayout->addWidget(m_fpsLabel);
    perfLayout->addWidget(m_memoryLabel);
    perfLayout->addWidget(m_loadTimeLabel);
    perfLayout->addWidget(m_averageLoadTimeLabel);
    perfLayout->addWidget(m_totalLoadsLabel);
    
    layout->addWidget(performanceGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_metricsTab, "Performance");
}

void DebugConsoleWidget::setupSystemTab()
{
    m_systemTab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(m_systemTab);
    
    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    m_refreshSystemButton = new QPushButton("Refresh System Info");
    m_refreshOpenGLButton = new QPushButton("Refresh OpenGL Info");
    connect(m_refreshSystemButton, &QPushButton::clicked, this, &DebugConsoleWidget::refreshSystemInfo);
    connect(m_refreshOpenGLButton, &QPushButton::clicked, this, &DebugConsoleWidget::refreshOpenGLInfo);
    
    buttonsLayout->addWidget(m_refreshSystemButton);
    buttonsLayout->addWidget(m_refreshOpenGLButton);
    buttonsLayout->addStretch();
    
    QGroupBox* systemGroup = new QGroupBox("System Information");
    QVBoxLayout* systemLayout = new QVBoxLayout(systemGroup);
    
    m_systemInfoTree = new QTreeWidget;
    m_systemInfoTree->setHeaderLabel("System Properties");
    systemLayout->addWidget(m_systemInfoTree);
    
    QGroupBox* openglGroup = new QGroupBox("OpenGL Information");
    QVBoxLayout* openglLayout = new QVBoxLayout(openglGroup);
    
    m_openglInfoTree = new QTreeWidget;
    m_openglInfoTree->setHeaderLabel("OpenGL Properties");
    openglLayout->addWidget(m_openglInfoTree);
    
    QGroupBox* pluginsGroup = new QGroupBox("Qt Plugins");
    QVBoxLayout* pluginsLayout = new QVBoxLayout(pluginsGroup);
    
    m_qtPluginsTree = new QTreeWidget;
    m_qtPluginsTree->setHeaderLabel("Available Plugins");
    pluginsLayout->addWidget(m_qtPluginsTree);
    
    layout->addLayout(buttonsLayout);
    layout->addWidget(systemGroup);
    layout->addWidget(openglGroup);
    layout->addWidget(pluginsGroup);
    
    m_tabWidget->addTab(m_systemTab, "System Info");
}

void DebugConsoleWidget::setupVTTTab()
{
    m_vttTab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(m_vttTab);
    
    QHBoxLayout* statusLayout = new QHBoxLayout;
    m_vttStatusLabel = new QLabel("VTT Status: Not loaded");
    m_refreshVTTButton = new QPushButton("Refresh VTT Info");
    connect(m_refreshVTTButton, &QPushButton::clicked, this, &DebugConsoleWidget::updateVTTDiagnostics);
    
    statusLayout->addWidget(m_vttStatusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_refreshVTTButton);
    
    QGroupBox* diagnosticsGroup = new QGroupBox("VTT File Diagnostics");
    QVBoxLayout* diagnosticsLayout = new QVBoxLayout(diagnosticsGroup);
    
    m_vttDiagnosticsTree = new QTreeWidget;
    m_vttDiagnosticsTree->setHeaderLabels({"Property", "Value", "Status"});
    diagnosticsLayout->addWidget(m_vttDiagnosticsTree);
    
    layout->addLayout(statusLayout);
    layout->addWidget(diagnosticsGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_vttTab, "VTT Diagnostics");
}

void DebugConsoleWidget::addMessage(const DebugMessage& message)
{
    if (messageMatchesFilter(message)) {
        m_filteredMessages.append(message);
        
        if (m_filteredMessages.size() > MAX_DISPLAY_MESSAGES) {
            m_filteredMessages.removeFirst();
        }
    }
}

void DebugConsoleWidget::clearMessages()
{
    m_logDisplay->clear();
    m_filteredMessages.clear();
}

void DebugConsoleWidget::onFilterChanged()
{
    m_currentFilter = m_filterCombo->currentText();
    updateDisplayedMessages();
}

void DebugConsoleWidget::onClearClicked()
{
    if (auto* console = DebugConsole::instance()) {
        console->clearMessages();
    }
}

void DebugConsoleWidget::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Debug Log", 
                                                   "debug_log.txt", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            if (auto* console = DebugConsole::instance()) {
                for (const DebugMessage& message : console->getMessages()) {
                    stream << QString("[%1] %2 [%3]: %4\n")
                              .arg(message.timestamp)
                              .arg(message.level)
                              .arg(message.category)
                              .arg(message.message);
                }
            }
            QMessageBox::information(this, "Export Complete", "Debug log exported successfully.");
        } else {
            QMessageBox::warning(this, "Export Failed", "Failed to write debug log to file.");
        }
    }
}

void DebugConsoleWidget::updateDisplayedMessages()
{
    m_filteredMessages.clear();

    if (auto* console = DebugConsole::instance()) {
        for (const DebugMessage& message : console->getMessages()) {
            if (messageMatchesFilter(message)) {
                m_filteredMessages.append(message);
            }
        }
    }
    
    m_logDisplay->clear();
    for (const DebugMessage& message : m_filteredMessages) {
        QString color = getLogLevelColor(message.level);
        QString backgroundColor = (message.level == "ERROR") ? "background-color: #2d1b1b; " : "";
        QString formattedMessage = QString("<span style='%1color: %2'>[%3] %4 [%5]: %6</span>")
                                  .arg(backgroundColor)
                                  .arg(color)
                                  .arg(message.timestamp)
                                  .arg(message.level)
                                  .arg(message.category)
                                  .arg(message.message);
        m_logDisplay->append(formattedMessage);
    }
    
    QScrollBar* scrollBar = m_logDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void DebugConsoleWidget::onMetricsUpdated(const PerformanceMetrics& metrics)
{
    updateMetricsDisplay();
}

void DebugConsoleWidget::updateMetricsDisplay()
{
    if (auto* console = DebugConsole::instance()) {
        const PerformanceMetrics& metrics = console->getMetrics();

        m_fpsLabel->setText(QString("FPS: %1").arg(metrics.fps, 0, 'f', 1));
        m_memoryLabel->setText(QString("Memory Usage: %1").arg(formatBytes(metrics.memoryUsage)));
        m_loadTimeLabel->setText(QString("Last Load Time: %1ms").arg(metrics.lastLoadTime));
        m_averageLoadTimeLabel->setText(QString("Average Load Time: %1ms").arg(qRound(metrics.averageLoadTime)));
        m_totalLoadsLabel->setText(QString("Total Loads: %1").arg(metrics.totalLoads));
    }
}

void DebugConsoleWidget::refreshSystemInfo()
{
    updateSystemInfoDisplay();
    updateQtPluginsDisplay();
}

void DebugConsoleWidget::refreshOpenGLInfo()
{
    updateOpenGLInfoDisplay();
}

void DebugConsoleWidget::updateSystemInfoDisplay()
{
    m_systemInfoTree->clear();

    auto* console = DebugConsole::instance();
    if (!console) return;

    const SystemInfo& info = console->getSystemInfo();

    // Add build info at the top (timestamp only in debug builds)
    QTreeWidgetItem* buildItem = new QTreeWidgetItem(m_systemInfoTree);
#ifdef QT_DEBUG
    buildItem->setText(0, QString("Build Timestamp: %1").arg(BUILD_TIMESTAMP));
#else
    buildItem->setText(0, QString("Build Version: 0.15.0-alpha"));
#endif
    buildItem->setForeground(0, QBrush(QColor("#4A90E2")));  // Highlight build info

    QTreeWidgetItem* qtItem = new QTreeWidgetItem(m_systemInfoTree);
    qtItem->setText(0, QString("Qt Version: %1").arg(info.qtVersion));
    
    QTreeWidgetItem* platformItem = new QTreeWidgetItem(m_systemInfoTree);
    platformItem->setText(0, QString("Platform: %1").arg(info.platformName));
    
    QTreeWidgetItem* cpuItem = new QTreeWidgetItem(m_systemInfoTree);
    cpuItem->setText(0, QString("CPU Architecture: %1").arg(info.cpuArchitecture));
    
    if (info.totalMemory > 0) {
        QTreeWidgetItem* memoryItem = new QTreeWidgetItem(m_systemInfoTree);
        memoryItem->setText(0, QString("Total Memory: %1").arg(formatBytes(info.totalMemory)));
    }
    
    m_systemInfoTree->expandAll();
}

void DebugConsoleWidget::updateOpenGLInfoDisplay()
{
    m_openglInfoTree->clear();

    auto* console = DebugConsole::instance();
    if (!console) return;

    const SystemInfo& info = console->getSystemInfo();
    
    QTreeWidgetItem* supportedItem = new QTreeWidgetItem(m_openglInfoTree);
    supportedItem->setText(0, QString("OpenGL Supported: %1").arg(info.openglSupported ? "Yes" : "No"));
    
    if (info.openglSupported) {
        QTreeWidgetItem* versionItem = new QTreeWidgetItem(m_openglInfoTree);
        versionItem->setText(0, QString("Version: %1").arg(info.openglVersion));
        
        QTreeWidgetItem* rendererItem = new QTreeWidgetItem(m_openglInfoTree);
        rendererItem->setText(0, QString("Renderer: %1").arg(info.openglRenderer));
    }
    
    m_openglInfoTree->expandAll();
}

void DebugConsoleWidget::updateQtPluginsDisplay()
{
    m_qtPluginsTree->clear();

    auto* console = DebugConsole::instance();
    if (!console) return;

    const SystemInfo& info = console->getSystemInfo();
    
    QMap<QString, QStringList> pluginsByCategory;
    for (const QString& plugin : info.availablePlugins) {
        QStringList parts = plugin.split('/');
        if (parts.size() == 2) {
            pluginsByCategory[parts[0]].append(parts[1]);
        }
    }
    
    for (auto it = pluginsByCategory.begin(); it != pluginsByCategory.end(); ++it) {
        QTreeWidgetItem* categoryItem = new QTreeWidgetItem(m_qtPluginsTree);
        categoryItem->setText(0, QString("%1 (%2)").arg(it.key()).arg(it.value().size()));
        
        for (const QString& plugin : it.value()) {
            QTreeWidgetItem* pluginItem = new QTreeWidgetItem(categoryItem);
            pluginItem->setText(0, plugin);
        }
    }
    
    m_qtPluginsTree->expandAll();
}

void DebugConsoleWidget::updateVTTDiagnostics()
{
    m_vttDiagnosticsTree->clear();
    
    QTreeWidgetItem* formatItem = new QTreeWidgetItem(m_vttDiagnosticsTree);
    formatItem->setText(0, "Supported VTT Formats");
    formatItem->setText(1, ".dd2vtt, .uvtt, .df2vtt");
    formatItem->setText(2, "Available");
    
    QTreeWidgetItem* imageItem = new QTreeWidgetItem(m_vttDiagnosticsTree);
    imageItem->setText(0, "Supported Image Formats");
    imageItem->setText(1, "PNG, JPG, WebP");
    imageItem->setText(2, "Available");
    
    m_vttDiagnosticsTree->expandAll();
}

QString DebugConsoleWidget::formatBytes(qint64 bytes) const
{
    if (bytes == 0) return "0 B";
    
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024 && unitIndex < 3) {
        size /= 1024;
        unitIndex++;
    }
    
    return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unitIndex]);
}

QString DebugConsoleWidget::getLogLevelColor(const QString& level) const
{
    if (level == "ERROR") return "#FFFFFF";
    if (level == "WARN") return "#FF8800";
    if (level == "PERF") return "#808080";
    if (level == "SYS") return "#808080";
    if (level == "VTT") return "#808080";
    if (level == "INFO") return "#0066CC";
    return "#000000";
}

bool DebugConsoleWidget::messageMatchesFilter(const DebugMessage& message) const
{
    return m_currentFilter == "All" || message.level == m_currentFilter;
}