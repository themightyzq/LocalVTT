#ifndef DEBUGCONSOLEWIDGET_H
#define DEBUGCONSOLEWIDGET_H

#include <QWidget>
#include <QTimer>
#include "utils/DebugConsole.h"

class QTextEdit;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QComboBox;
class QPushButton;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QGroupBox;

class DebugConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DebugConsoleWidget(QWidget* parent = nullptr);
    ~DebugConsoleWidget() = default;

    void addMessage(const DebugMessage& message);
    void clearMessages();

public slots:
    void onMetricsUpdated(const PerformanceMetrics& metrics);

private slots:
    void onFilterChanged();
    void onClearClicked();
    void onExportClicked();
    void updateDisplayedMessages();
    void refreshSystemInfo();
    void refreshOpenGLInfo();

private:
    void setupUI();
    void setupLogTab();
    void setupMetricsTab();
    void setupSystemTab();
    void setupVTTTab();
    void updateMetricsDisplay();
    void updateSystemInfoDisplay();
    void updateOpenGLInfoDisplay();
    void updateQtPluginsDisplay();
    void updateVTTDiagnostics();
    QString formatBytes(qint64 bytes) const;
    QString getLogLevelColor(const QString& level) const;
    bool messageMatchesFilter(const DebugMessage& message) const;

    QTabWidget* m_tabWidget;
    
    QWidget* m_logTab;
    QTextEdit* m_logDisplay;
    QComboBox* m_filterCombo;
    QPushButton* m_clearButton;
    QPushButton* m_exportButton;
    
    QWidget* m_metricsTab;
    QLabel* m_fpsLabel;
    QLabel* m_memoryLabel;
    QLabel* m_loadTimeLabel;
    QLabel* m_averageLoadTimeLabel;
    QLabel* m_totalLoadsLabel;
    
    QWidget* m_systemTab;
    QTreeWidget* m_systemInfoTree;
    QTreeWidget* m_openglInfoTree;
    QTreeWidget* m_qtPluginsTree;
    QPushButton* m_refreshSystemButton;
    QPushButton* m_refreshOpenGLButton;
    
    QWidget* m_vttTab;
    QLabel* m_vttStatusLabel;
    QTreeWidget* m_vttDiagnosticsTree;
    QPushButton* m_refreshVTTButton;
    
    QTimer* m_updateTimer;
    QString m_currentFilter;
    QList<DebugMessage> m_filteredMessages;
    
    static const int MAX_DISPLAY_MESSAGES = 500;
    static const int UPDATE_INTERVAL = 100;
};

#endif