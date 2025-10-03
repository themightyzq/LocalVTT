#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QString>
#include <QImage>
#include <QJsonObject>
#include <QObject>

class ImageLoader : public QObject
{
    Q_OBJECT

public:
    explicit ImageLoader(QObject* parent = nullptr);

    // Load image from file
    static QImage loadImage(const QString& path);

    // Load image with progress reporting
    QImage loadImageWithProgress(const QString& path);

    // Load UVTT format (JSON with image data)
    static bool loadUVTT(const QString& path, QImage& outImage, QJsonObject& outMetadata);

    // Save UVTT format
    static bool saveUVTT(const QString& path, const QImage& image, const QJsonObject& metadata);

    // Check if file is UVTT format
    static bool isUVTTFile(const QString& path);

signals:
    void progressChanged(int percentage);
    void statusChanged(const QString& status);

private:
    static QByteArray compressImage(const QImage& image);
    static QImage decompressImage(const QByteArray& data);

    // Helper for progress reporting
    void reportProgress(int percentage, const QString& status);
};

#endif // IMAGELOADER_H
