#include "controllers/FileOperationsManager.h"
#include "utils/DebugConsole.h"
#include "ui/MainWindow.h"
#include "graphics/MapDisplay.h"
#include "graphics/FogOfWar.h"
#include "utils/DebugConsole.h"
#include <QFileDialog>
#include <QProgressDialog>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QUrl>

FileOperationsManager::FileOperationsManager(MainWindow* mainWindow, MapDisplay* mapDisplay, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_mapDisplay(mapDisplay)
    , m_progressDialog(nullptr)
    , m_dropOverlay(nullptr)
    , m_dropAnimation(nullptr)
    , m_isDragging(false)
{
    connect(this, &FileOperationsManager::dropAnimationRequested,
            this, &FileOperationsManager::animateDropFeedback);
}

void FileOperationsManager::loadMap()
{
    QString fileName = QFileDialog::getOpenFileName(
        m_mainWindow,
        "Load Map Image",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Map Files (*.png *.jpg *.jpeg *.webp *.dd2vtt *.uvtt *.df2vtt);;Images (*.png *.jpg *.jpeg *.webp);;VTT Files (*.dd2vtt *.uvtt *.df2vtt);;All Files (*.*)"
    );

    if (!fileName.isEmpty()) {
        loadMapFile(fileName);
    }
}

void FileOperationsManager::loadMapFile(const QString& path)
{
    if (!m_mapDisplay) return;

    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        DebugConsole::error(QString("File does not exist: %1").arg(path), "FileOperations");
        return;
    }

    m_loadTimer.start();
    showLoadProgress(fileInfo.fileName(), fileInfo.size());

    // Progress updates no longer needed with fast loading

    bool success = m_mapDisplay->loadImage(path);

    hideLoadProgress();

    if (success) {
        m_currentMapPath = path;
        emit mapLoaded(path);

        loadFogState(path);

        qint64 loadTime = m_loadTimer.elapsed();
        DebugConsole::info(
            QString("Map loaded successfully: %1 (%2 ms)")
            .arg(fileInfo.fileName())
            .arg(loadTime), "FileOperations");
    } else {
        DebugConsole::error(
            QString("Failed to load map: %1").arg(path), "FileOperations");
    }
}

void FileOperationsManager::loadMapFromCommandLine(const QString& path)
{
    loadMapFile(path);
}

void FileOperationsManager::handleDragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        const auto urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            const QString fileName = urls.first().toLocalFile();
            const QString suffix = QFileInfo(fileName).suffix().toLower();

            if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
                suffix == "webp" || suffix == "dd2vtt" || suffix == "uvtt" ||
                suffix == "df2vtt") {
                event->acceptProposedAction();

                if (!m_isDragging) {
                    m_isDragging = true;
                    emit dropAnimationRequested(true);
                }
                return;
            }
        }
    }
    event->ignore();
}

void FileOperationsManager::handleDragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void FileOperationsManager::handleDragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event)
    if (m_isDragging) {
        m_isDragging = false;
        emit dropAnimationRequested(false);
    }
}

void FileOperationsManager::handleDropEvent(QDropEvent* event)
{
    if (m_isDragging) {
        m_isDragging = false;
        emit dropAnimationRequested(false);
    }

    const auto urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        const QString fileName = urls.first().toLocalFile();
        loadMapFile(fileName);
        event->acceptProposedAction();
    }
}

QString FileOperationsManager::getFogFilePath(const QString& mapPath) const
{
    if (mapPath.isEmpty()) return QString();

    QFileInfo fileInfo(mapPath);
    QString baseName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();

    QString fogDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/fog_states";
    QDir().mkpath(fogDir);

    QString safeFileName = baseName + "_" +
                          QString::number(qHash(mapPath)) + ".fog";

    return fogDir + "/" + safeFileName;
}

void FileOperationsManager::loadFogState(const QString& mapPath)
{
    if (!m_mapDisplay || mapPath.isEmpty()) return;

    QString fogPath = getFogFilePath(mapPath);
    if (fogPath.isEmpty()) return;

    QFile file(fogPath);
    if (file.exists()) {
        if (m_mapDisplay->getFogOverlay()) {
            QFile fogFile(fogPath);
            if (fogFile.open(QIODevice::ReadOnly)) {
                QByteArray fogData = fogFile.readAll();
                m_mapDisplay->getFogOverlay()->loadState(fogData);
                fogFile.close();
                emit fogStateLoaded(mapPath);
                DebugConsole::info(
                    QString("Loaded fog state from: %1").arg(fogPath), "FileOperations");
            }
        }
    }
}

void FileOperationsManager::saveFogState()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay() || m_currentMapPath.isEmpty()) {
        return;
    }

    QString fogPath = getFogFilePath(m_currentMapPath);
    if (fogPath.isEmpty()) return;

    QByteArray fogData = m_mapDisplay->getFogOverlay()->saveState();
    QFile fogFile(fogPath);
    if (fogFile.open(QIODevice::WriteOnly)) {
        fogFile.write(fogData);
        fogFile.close();
        emit fogStateSaved();
        showAutosaveNotification("Fog state saved");
        DebugConsole::info(
            QString("Saved fog state to: %1").arg(fogPath), "FileOperations");
    }
}

void FileOperationsManager::quickSaveFogState()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay()) return;

    QString quickSaveDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/quicksaves";
    QDir().mkpath(quickSaveDir);

    m_quickSavePath = quickSaveDir + "/quicksave.fog";

    QByteArray fogData = m_mapDisplay->getFogOverlay()->saveState();
    QFile fogFile(m_quickSavePath);
    if (fogFile.open(QIODevice::WriteOnly)) {
        fogFile.write(fogData);
        fogFile.close();
        showAutosaveNotification("Quick save completed");
        DebugConsole::info("Quick save completed", "FileOperations");
    }
}

void FileOperationsManager::quickRestoreFogState()
{
    if (!m_mapDisplay || !m_mapDisplay->getFogOverlay() || m_quickSavePath.isEmpty()) {
        return;
    }

    QFile file(m_quickSavePath);
    if (file.exists()) {
        QFile fogFile(m_quickSavePath);
        if (fogFile.open(QIODevice::ReadOnly)) {
            QByteArray fogData = fogFile.readAll();
            fogFile.close();
            if (m_mapDisplay->getFogOverlay()->loadState(fogData)) {
                showAutosaveNotification("Quick restore completed");
                DebugConsole::info("Quick restore completed", "FileOperations");
            }
        }
    } else {
        showAutosaveNotification("No quick save found");
    }
}

void FileOperationsManager::handleLoadProgress(qint64 bytesRead, qint64 totalBytes)
{
    if (m_progressDialog && m_progressDialog->isVisible()) {
        m_progressDialog->setMaximum(totalBytes);
        m_progressDialog->setValue(bytesRead);

        double percentage = (totalBytes > 0) ? (bytesRead * 100.0 / totalBytes) : 0;
        m_progressDialog->setLabelText(QString("Loading map... %1%").arg(percentage, 0, 'f', 1));

        emit loadProgressUpdate(bytesRead, totalBytes);
    }
}

void FileOperationsManager::showLoadProgress(const QString& fileName, qint64 fileSize)
{
    if (fileSize > 5 * 1024 * 1024) {
        if (!m_progressDialog) {
            m_progressDialog = new QProgressDialog(m_mainWindow);
            m_progressDialog->setWindowModality(Qt::WindowModal);
            m_progressDialog->setMinimumDuration(500);
        }

        m_progressDialog->setLabelText(QString("Loading %1...").arg(fileName));
        m_progressDialog->setRange(0, fileSize);
        m_progressDialog->setValue(0);
        m_progressDialog->show();
    }
}

void FileOperationsManager::hideLoadProgress()
{
    if (m_progressDialog && m_progressDialog->isVisible()) {
        m_progressDialog->hide();
    }
}

void FileOperationsManager::createDropOverlay()
{
    if (!m_dropOverlay) {
        m_dropOverlay = new QWidget(m_mainWindow);
        m_dropOverlay->setObjectName("dropOverlay");
        m_dropOverlay->setStyleSheet(R"(
            #dropOverlay {
                background-color: rgba(42, 82, 152, 180);
                border: 3px dashed #4a9eff;
                border-radius: 10px;
            }
        )");

        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(m_dropOverlay);
        m_dropOverlay->setGraphicsEffect(effect);

        m_dropAnimation = new QPropertyAnimation(effect, "opacity", this);
        m_dropAnimation->setDuration(200);
    }
}

void FileOperationsManager::animateDropFeedback(bool entering)
{
    createDropOverlay();

    if (entering) {
        m_dropOverlay->setGeometry(m_mainWindow->rect().adjusted(10, 10, -10, -10));
        m_dropOverlay->show();
        m_dropOverlay->raise();

        m_dropAnimation->setStartValue(0.0);
        m_dropAnimation->setEndValue(1.0);
        m_dropAnimation->start();
    } else {
        m_dropAnimation->setStartValue(1.0);
        m_dropAnimation->setEndValue(0.0);
        connect(m_dropAnimation, &QPropertyAnimation::finished, [this]() {
            if (m_dropOverlay) {
                m_dropOverlay->hide();
            }
        });
        m_dropAnimation->start();
    }
}

void FileOperationsManager::scheduleFogSave()
{
    QTimer::singleShot(1000, this, &FileOperationsManager::saveFogState);
}

void FileOperationsManager::showAutosaveNotification(const QString& message)
{
    emit autosaveNotificationRequested(message);
}