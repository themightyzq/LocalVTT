#ifndef MAPIMAGEMANAGER_H
#define MAPIMAGEMANAGER_H

#include <QObject>
#include <QImage>
#include "utils/VTTLoader.h"

class ImageLoader;
class LoadingProgressWidget;
class QWidget;

class MapImageManager : public QObject
{
    Q_OBJECT

public:
    explicit MapImageManager(QWidget* parentWidget, QObject* parent = nullptr);
    ~MapImageManager();

    bool loadImage(const QString& path);
    bool loadImageWithProgress(const QString& path);
    bool loadImageFromCache(const QImage& cachedImage, const VTTLoader::VTTData& vttData);
    void setCachedImage(const QImage& image);

    const QImage& getCurrentImage() const { return m_currentImage; }
    const VTTLoader::VTTData& getVttData() const { return m_vttData; }
    bool hasVttData() const { return m_hasVttData; }
    int getVttGridSize() const { return m_vttGridSize; }

    static void setAppReadyForProgress(bool ready) { s_appReadyForProgress = ready; }

signals:
    void imageLoaded(const QImage& image, int vttGridSize);
    void imageLoadFailed(const QString& error);
    void progressUpdate(int percentage, const QString& message);
    void vttDataLoaded(const VTTLoader::VTTData& data);

private:
    void showProgressWidget();
    void hideProgressWidget();
    void processLoadedImage(const QImage& image, const QString& path);
    void processVttData(const VTTLoader::VTTData& data);

    QWidget* m_parentWidget;
    LoadingProgressWidget* m_loadingProgressWidget;
    ImageLoader* m_imageLoader;

    QImage m_currentImage;
    VTTLoader::VTTData m_vttData;
    bool m_hasVttData;
    int m_vttGridSize;

    static bool s_appReadyForProgress;
};

#endif // MAPIMAGEMANAGER_H