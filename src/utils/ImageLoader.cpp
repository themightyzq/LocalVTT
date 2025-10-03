#include "utils/ImageLoader.h"
#include "utils/VTTLoader.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBuffer>
#include <QImageReader>
#include <QImageWriter>
#include <QFileInfo>
#include <QThread>
#include <QApplication>
#include <QEventLoop>

ImageLoader::ImageLoader(QObject* parent)
    : QObject(parent)
{
}

QImage ImageLoader::loadImageWithProgress(const QString& path)
{
    reportProgress(0, "Starting image load...");

    // Get file size to determine if we need chunked loading
    QFileInfo fileInfo(path);
    qint64 fileSize = fileInfo.size();
    const qint64 LARGE_FILE_THRESHOLD = 10 * 1024 * 1024; // 10MB

    // Check if it's a VTT file first
    if (VTTLoader::isVTTFile(path)) {
        reportProgress(25, "Loading VTT file...");

        // Process events frequently for large files
        if (fileSize > LARGE_FILE_THRESHOLD) {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        // Pass progress callback to VTTLoader for unified progress reporting
        auto progressCallback = [this](int percentage, const QString& message) {
            reportProgress(percentage, message);
        };

        VTTLoader::VTTData vttData = VTTLoader::loadVTT(path, progressCallback);

        reportProgress(75, "Processing VTT data...");
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!vttData.mapImage.isNull()) {
            reportProgress(100, "VTT file loaded successfully");
            return vttData.mapImage;
        } else {
            reportProgress(100, "Failed to load VTT file");
            return QImage();
        }
    } else {
        // Load regular image with better progress reporting
        reportProgress(10, "Opening image file...");

        QImageReader reader(path);
        reader.setAutoTransform(true);

        // For very large files, enable incremental reading
        if (fileSize > LARGE_FILE_THRESHOLD) {
            reader.setDecideFormatFromContent(true);
        }

        reportProgress(25, "Reading image data...");
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        // Check if the format is supported before attempting to read
        if (!reader.canRead()) {
            reportProgress(100, "Unsupported image format");
            return QImage();
        }

        reportProgress(50, "Decoding image data...");

        // For large files, process events more frequently
        if (fileSize > LARGE_FILE_THRESHOLD) {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        QImage image = reader.read();

        if (!image.isNull()) {
            reportProgress(75, "Finalizing image...");
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

            // Memory optimization: Only convert format if absolutely necessary
            // RGB888 uses 25% less memory than ARGB32 for images without alpha
            if (image.hasAlphaChannel()) {
                // Only convert to ARGB32 if we actually have alpha
                if (image.format() != QImage::Format_ARGB32 &&
                    image.format() != QImage::Format_ARGB32_Premultiplied) {
                    reportProgress(85, "Optimizing image format with alpha...");
                    image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
                    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                }
            } else {
                // For images without alpha, use RGB888 to save memory
                if (image.format() != QImage::Format_RGB888 &&
                    image.format() != QImage::Format_RGB32) {
                    reportProgress(85, "Optimizing image format...");
                    image = image.convertToFormat(QImage::Format_RGB888);
                    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                }
            }

            reportProgress(100, "Image loaded successfully");
        } else {
            QString errorString = reader.errorString();
            reportProgress(100, QString("Failed to load image: %1").arg(errorString));
        }

        return image;
    }
}

QImage ImageLoader::loadImage(const QString& path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    return reader.read();
}

bool ImageLoader::loadUVTT(const QString& path, QImage& outImage, QJsonObject& outMetadata)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();

    // Extract metadata
    outMetadata = root;

    // Extract image data (base64 encoded)
    if (root.contains("image") && root["image"].isString()) {
        QString base64Data = root["image"].toString();
        QByteArray imageData = QByteArray::fromBase64(base64Data.toUtf8());
        outImage = decompressImage(imageData);
        return !outImage.isNull();
    }

    return false;
}

bool ImageLoader::saveUVTT(const QString& path, const QImage& image, const QJsonObject& metadata)
{
    QJsonObject root = metadata;

    // Add image data
    QByteArray imageData = compressImage(image);
    QString base64Data = QString::fromUtf8(imageData.toBase64());
    root["image"] = base64Data;

    // Save to file
    QJsonDocument doc(root);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    file.close();

    return true;
}

bool ImageLoader::isUVTTFile(const QString& path)
{
    return VTTLoader::isVTTFile(path);
}

QByteArray ImageLoader::compressImage(const QImage& image)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);

    QImageWriter writer(&buffer, "PNG");
    writer.setCompression(9);  // Max compression
    writer.write(image);

    return data;
}

QImage ImageLoader::decompressImage(const QByteArray& data)
{
    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);

    QImageReader reader(&buffer);
    return reader.read();
}

void ImageLoader::reportProgress(int percentage, const QString& status)
{
    emit progressChanged(percentage);
    emit statusChanged(status);

    // Allow UI to update
    QApplication::processEvents();
}
