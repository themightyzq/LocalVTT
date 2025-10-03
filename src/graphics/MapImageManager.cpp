#include "MapImageManager.h"
#include "LoadingProgressWidget.h"
#include "utils/ImageLoader.h"
#include "utils/VTTLoader.h"
#include "utils/DebugConsole.h"
#include <QFileInfo>
#include <QApplication>
#include <QThread>

bool MapImageManager::s_appReadyForProgress = false;

MapImageManager::MapImageManager(QWidget* parentWidget, QObject* parent)
    : QObject(parent)
    , m_parentWidget(parentWidget)
    , m_loadingProgressWidget(nullptr)
    , m_imageLoader(nullptr)
    , m_hasVttData(false)
    , m_vttGridSize(0)
{
    m_loadingProgressWidget = new LoadingProgressWidget(m_parentWidget);
    m_loadingProgressWidget->hide();

    m_imageLoader = new ImageLoader(this);
    connect(m_imageLoader, &ImageLoader::progressChanged, this, [this](int percentage) {
        if (m_loadingProgressWidget && m_loadingProgressWidget->isVisible()) {
            m_loadingProgressWidget->setProgress(percentage);
        }
        emit progressUpdate(percentage, QString());
    });

    connect(m_imageLoader, &ImageLoader::statusChanged, this, [this](const QString& status) {
        if (m_loadingProgressWidget && m_loadingProgressWidget->isVisible()) {
            m_loadingProgressWidget->setLoadingText(status);
        }
        emit progressUpdate(0, status);
    });
}

MapImageManager::~MapImageManager()
{
    // ImageLoader is not a thread, nothing to cancel
}

bool MapImageManager::loadImage(const QString& path)
{
    if (path.isEmpty()) return false;

    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        emit imageLoadFailed(QString("File not found: %1").arg(path));
        return false;
    }

    QString ext = fileInfo.suffix().toLower();
    m_hasVttData = false;
    m_vttGridSize = 0;

    if (ext == "dd2vtt" || ext == "uvtt" || ext == "df2vtt") {
        VTTLoader vttLoader;
        VTTLoader::VTTData vttData = vttLoader.loadVTT(path);

        if (!vttData.isValid) {
            emit imageLoadFailed(QString("Failed to load VTT file: %1").arg(path));
            return false;
        }

        processVttData(vttData);

        if (vttData.mapImage.isNull()) {
            emit imageLoadFailed(QString("VTT file contains no valid image: %1").arg(path));
            return false;
        }

        // Memory optimization: Move image instead of copying
        m_currentImage = std::move(vttData.mapImage);
        emit imageLoaded(m_currentImage, m_vttGridSize);

        DebugConsole::info(QString("VTT file loaded: %1x%2 pixels, grid size: %3")
            .arg(m_currentImage.width())
            .arg(m_currentImage.height())
            .arg(m_vttGridSize), "VTT");

        return true;
    } else {
        QImage image = m_imageLoader->loadImageWithProgress(path);
        if (!image.isNull()) {
            processLoadedImage(image, path);
            return true;
        }
        emit imageLoadFailed(QString("Failed to load image: %1").arg(path));
        return false;
    }
}

bool MapImageManager::loadImageWithProgress(const QString& path)
{
    if (!s_appReadyForProgress || path.isEmpty()) {
        return loadImage(path);
    }

    showProgressWidget();
    return loadImage(path);
}

bool MapImageManager::loadImageFromCache(const QImage& cachedImage, const VTTLoader::VTTData& vttData)
{
    if (cachedImage.isNull()) return false;

    m_currentImage = cachedImage;
    processVttData(vttData);

    emit imageLoaded(m_currentImage, m_vttGridSize);
    return true;
}

void MapImageManager::setCachedImage(const QImage& image)
{
    // Memory optimization: Clear old image before setting new one
    m_currentImage = QImage();
    m_currentImage = image;
}

void MapImageManager::showProgressWidget()
{
    if (m_loadingProgressWidget && s_appReadyForProgress) {
        m_loadingProgressWidget->show();
        m_loadingProgressWidget->raise();
        m_loadingProgressWidget->setProgress(0);
        m_loadingProgressWidget->setLoadingText("Loading map...");
        QApplication::processEvents();
    }
}

void MapImageManager::hideProgressWidget()
{
    if (m_loadingProgressWidget) {
        m_loadingProgressWidget->hide();
    }
}

void MapImageManager::processLoadedImage(const QImage& image, const QString& path)
{
    hideProgressWidget();

    if (image.isNull()) {
        emit imageLoadFailed(QString("Failed to load image: %1").arg(path));
        return;
    }

    // Memory optimization: Move image instead of copying
    m_currentImage = std::move(const_cast<QImage&>(image));
    emit imageLoaded(m_currentImage, m_vttGridSize);

    DebugConsole::info(QString("Image loaded: %1x%2 pixels")
        .arg(image.width()).arg(image.height()), "Graphics");
}

void MapImageManager::processVttData(const VTTLoader::VTTData& data)
{
    m_vttData = data;
    m_hasVttData = data.isValid;

    if (m_hasVttData) {
        m_vttGridSize = data.pixelsPerGrid;
        emit vttDataLoaded(data);

        if (data.globalLight || data.darkness > 0) {
            DebugConsole::info(QString("VTT lighting: Global light=%1, Darkness=%2")
                .arg(data.globalLight)
                .arg(data.darkness), "VTT");
        }

        if (!data.lights.isEmpty()) {
            DebugConsole::info(QString("VTT contains %1 light sources").arg(data.lights.size()), "VTT");
        }

        if (!data.walls.isEmpty()) {
            DebugConsole::info(QString("VTT contains %1 wall segments").arg(data.walls.size()), "VTT");
        }

        if (!data.portals.isEmpty()) {
            DebugConsole::info(QString("VTT contains %1 portal segments").arg(data.portals.size()), "VTT");
        }
    }
}