#include "utils/VTTLoader.h"
#include "utils/DebugConsole.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QByteArray>
#include <QBuffer>
#include <QImageReader>
#include <QApplication>
#include <QThread>
#include <QRegularExpression>

VTTLoader::VTTData VTTLoader::loadVTT(const QString& filepath, ProgressCallback progressCallback)
{
    VTTData data;

    // Helper to report progress safely
    auto reportProgress = [&](int percentage, const QString& message) {
        if (progressCallback) {
            progressCallback(percentage, message);
        }
    };

    reportProgress(0, "Opening VTT file...");

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        data.errorMessage = QString("Failed to open file: %1").arg(filepath);
        DebugConsole::error(QString("Failed to open file: %1").arg(filepath), "VTT");
        reportProgress(100, "Failed to open file");
        return data;
    }

    // Check file size before reading to prevent extreme cases
    qint64 fileSize = file.size();
    const qint64 MAX_FILE_SIZE = 1024LL * 1024LL * 1024LL; // 1GB safety cap
    if (fileSize > MAX_FILE_SIZE) {
        data.errorMessage = QString("File too large: %1 MB (max: %2 MB)")
            .arg(fileSize / (1024.0 * 1024.0), 0, 'f', 1)
            .arg(MAX_FILE_SIZE / (1024.0 * 1024.0));
        DebugConsole::error(data.errorMessage, "VTT");
        file.close();
        reportProgress(100, "File too large");
        return data;
    }

    reportProgress(10, "Reading file data...");
    QByteArray fileData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(fileData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        data.errorMessage = QString("JSON parse error at offset %1: %2")
            .arg(parseError.offset)
            .arg(parseError.errorString());
        DebugConsole::error(data.errorMessage, "VTT");
        reportProgress(100, "JSON parse failed");
        return data;
    }

    QJsonObject root = doc.object();
    DebugConsole::vtt(QString("Successfully parsed JSON with %1 keys: %2").arg(root.keys().size()).arg(root.keys().join(", ")), "VTT");
    reportProgress(20, "Parsing VTT metadata...");

    // Extract resolution/grid information with validation
    if (root.contains("resolution") && root["resolution"].isObject()) {
        QJsonObject resolution = root["resolution"].toObject();

        if (resolution.contains("map_size") && resolution["map_size"].isObject()) {
            QJsonObject mapSize = resolution["map_size"].toObject();
            int gridX = mapSize.contains("x") ? mapSize["x"].toInt() : 0;
            int gridY = mapSize.contains("y") ? mapSize["y"].toInt() : 0;

            // Validate grid dimensions to prevent integer overflow
            const int MAX_GRID_SQUARES = 1000;
            if (gridX > 0 && gridX <= MAX_GRID_SQUARES &&
                gridY > 0 && gridY <= MAX_GRID_SQUARES) {
                data.gridSquaresX = gridX;
                data.gridSquaresY = gridY;
            } else {
                DebugConsole::warning(QString("Grid dimensions out of range: %1x%2").arg(gridX).arg(gridY), "VTT");
            }
        }

        if (resolution.contains("pixels_per_grid")) {
            int pixelsPerGrid = resolution["pixels_per_grid"].toInt();

            // Validate pixels per grid
            const int MAX_PIXELS_PER_GRID = 500;
            if (pixelsPerGrid > 0 && pixelsPerGrid <= MAX_PIXELS_PER_GRID) {
                data.pixelsPerGrid = pixelsPerGrid;
            } else {
                DebugConsole::warning(QString("Pixels per grid out of range: %1, using default 50").arg(pixelsPerGrid), "VTT");
                data.pixelsPerGrid = 50;
            }
        }
    }

    // Extract environment data
    if (root.contains("environment") && root["environment"].isObject()) {
        QJsonObject environment = root["environment"].toObject();

        if (environment.contains("ambient_light") && environment["ambient_light"].isString()) {
            QString hexColor = environment["ambient_light"].toString();
            data.ambientLight = parseHexColor(hexColor);
        }
    }

    // Extract Foundry lighting data
    if (root.contains("globalLight")) {
        data.globalLight = root["globalLight"].toBool();
    }

    if (root.contains("darkness")) {
        data.darkness = qBound(0.0, root["darkness"].toDouble(), 1.0);
    }

    // Extract line of sight walls
    if (root.contains("line_of_sight") && root["line_of_sight"].isArray()) {
        QJsonArray wallsArray = root["line_of_sight"].toArray();
        DebugConsole::vtt(QString("Found %1 wall segments").arg(wallsArray.size()), "VTT");

        const int MAX_WALLS = 10000;
        int wallCount = 0;

        for (const QJsonValue& wallValue : wallsArray) {
            if (wallCount >= MAX_WALLS) {
                DebugConsole::warning(QString("Too many walls, limiting to %1").arg(MAX_WALLS), "VTT");
                break;
            }

            if (!wallValue.isArray()) {
                DebugConsole::warning("Invalid wall entry, expecting array, skipping", "VTT");
                continue;
            }

            QJsonArray wallPointsArray = wallValue.toArray();
            if (wallPointsArray.size() != 2) {
                DebugConsole::warning(QString("Wall must have exactly 2 points, found %1, skipping").arg(wallPointsArray.size()), "VTT");
                continue;
            }

            bool validWall = true;
            QPointF startPoint, endPoint;

            for (int i = 0; i < 2; ++i) {
                if (!wallPointsArray[i].isObject()) {
                    DebugConsole::warning(QString("Wall point %1 is not an object, skipping wall").arg(i), "VTT");
                    validWall = false;
                    break;
                }

                QJsonObject pointObj = wallPointsArray[i].toObject();
                if (!pointObj.contains("x") || !pointObj.contains("y")) {
                    DebugConsole::warning(QString("Wall point %1 missing x or y coordinate, skipping wall").arg(i), "VTT");
                    validWall = false;
                    break;
                }

                qreal x = pointObj["x"].toDouble();
                qreal y = pointObj["y"].toDouble();

                if (!qIsFinite(x) || !qIsFinite(y)) {
                    DebugConsole::warning(QString("Wall point %1 has invalid coordinates, skipping wall").arg(i), "VTT");
                    validWall = false;
                    break;
                }

                const qreal MAX_COORD = 100000.0;
                if (qAbs(x) > MAX_COORD || qAbs(y) > MAX_COORD) {
                    DebugConsole::warning(QString("Wall point %1 coordinates out of range, skipping wall").arg(i), "VTT");
                    validWall = false;
                    break;
                }

                if (i == 0) {
                    startPoint = QPointF(x, y);
                } else {
                    endPoint = QPointF(x, y);
                }
            }

            if (validWall) {
                if (QLineF(startPoint, endPoint).length() < 0.001) {
                    DebugConsole::warning("Wall segment too short, skipping", "VTT");
                    continue;
                }

                VTTLoader::WallSegment wall(startPoint, endPoint);
                data.walls.append(wall);
                wallCount++;
            }
        }

        DebugConsole::vtt(QString("Successfully loaded %1 wall segments").arg(data.walls.size()), "VTT");
    }

    // Extract portals data
    if (root.contains("portals") && root["portals"].isArray()) {
        QJsonArray portalsArray = root["portals"].toArray();
        DebugConsole::vtt(QString("Found %1 portal entries").arg(portalsArray.size()), "VTT");

        const int MAX_PORTALS = 1000;
        int portalCount = 0;

        for (const QJsonValue& portalValue : portalsArray) {
            if (portalCount >= MAX_PORTALS) {
                DebugConsole::warning(QString("Too many portals, limiting to %1").arg(MAX_PORTALS), "VTT");
                break;
            }

            if (!portalValue.isObject()) {
                DebugConsole::warning("Invalid portal entry, expecting object, skipping", "VTT");
                continue;
            }

            QJsonObject portalObj = portalValue.toObject();

            // Validate required fields
            if (!portalObj.contains("position") || !portalObj.contains("bounds")) {
                DebugConsole::warning("Portal missing required fields (position/bounds), skipping", "VTT");
                continue;
            }

            // Parse position
            QJsonObject positionObj = portalObj["position"].toObject();
            if (!positionObj.contains("x") || !positionObj.contains("y")) {
                DebugConsole::warning("Portal position missing x or y coordinate, skipping", "VTT");
                continue;
            }

            qreal posX = positionObj["x"].toDouble();
            qreal posY = positionObj["y"].toDouble();

            if (!qIsFinite(posX) || !qIsFinite(posY)) {
                DebugConsole::warning("Portal position has invalid coordinates, skipping", "VTT");
                continue;
            }

            const qreal MAX_COORD = 100000.0;
            if (qAbs(posX) > MAX_COORD || qAbs(posY) > MAX_COORD) {
                DebugConsole::warning("Portal position coordinates out of range, skipping", "VTT");
                continue;
            }

            QPointF position(posX, posY);

            // Parse bounds array
            if (!portalObj["bounds"].isArray()) {
                DebugConsole::warning("Portal bounds is not an array, skipping", "VTT");
                continue;
            }

            QJsonArray boundsArray = portalObj["bounds"].toArray();
            if (boundsArray.size() != 2) {
                DebugConsole::warning(QString("Portal bounds must have exactly 2 points, found %1, skipping").arg(boundsArray.size()), "VTT");
                continue;
            }

            bool validBounds = true;
            QPointF bound1, bound2;

            for (int i = 0; i < 2; ++i) {
                if (!boundsArray[i].isObject()) {
                    DebugConsole::warning(QString("Portal bound %1 is not an object, skipping portal").arg(i), "VTT");
                    validBounds = false;
                    break;
                }

                QJsonObject boundObj = boundsArray[i].toObject();
                if (!boundObj.contains("x") || !boundObj.contains("y")) {
                    DebugConsole::warning(QString("Portal bound %1 missing x or y coordinate, skipping portal").arg(i), "VTT");
                    validBounds = false;
                    break;
                }

                qreal x = boundObj["x"].toDouble();
                qreal y = boundObj["y"].toDouble();

                if (!qIsFinite(x) || !qIsFinite(y)) {
                    DebugConsole::warning(QString("Portal bound %1 has invalid coordinates, skipping portal").arg(i), "VTT");
                    validBounds = false;
                    break;
                }

                if (qAbs(x) > MAX_COORD || qAbs(y) > MAX_COORD) {
                    DebugConsole::warning(QString("Portal bound %1 coordinates out of range, skipping portal").arg(i), "VTT");
                    validBounds = false;
                    break;
                }

                if (i == 0) {
                    bound1 = QPointF(x, y);
                } else {
                    bound2 = QPointF(x, y);
                }
            }

            if (!validBounds) {
                continue;
            }

            // Check minimum distance between bounds
            if (QLineF(bound1, bound2).length() < 0.001) {
                DebugConsole::warning("Portal bounds too close together, skipping", "VTT");
                continue;
            }

            // Parse optional fields
            qreal rotation = portalObj.contains("rotation") ?
                qBound(-10.0, portalObj["rotation"].toDouble(), 10.0) : 0.0;
            bool closed = portalObj.contains("closed") ?
                portalObj["closed"].toBool() : false;
            bool freestanding = portalObj.contains("freestanding") ?
                portalObj["freestanding"].toBool() : false;

            PortalData portal(position, bound1, bound2, rotation, closed, freestanding);
            data.portals.append(portal);
            portalCount++;
        }

        DebugConsole::vtt(QString("Successfully loaded %1 portals").arg(data.portals.size()), "VTT");
    }

    if (root.contains("lights") && root["lights"].isArray()) {
        QJsonArray lightsArray = root["lights"].toArray();
        DebugConsole::vtt(QString("Found %1 light sources").arg(lightsArray.size()), "VTT");

        const int MAX_LIGHTS = 1000;  // Prevent excessive memory allocation
        int lightCount = 0;
        int skippedCount = 0;

        for (const QJsonValue& lightValue : lightsArray) {
            if (lightCount >= MAX_LIGHTS) {
                DebugConsole::warning(QString("Too many lights, limiting to %1").arg(MAX_LIGHTS), "VTT");
                break;
            }

            if (!lightValue.isObject()) {
                DebugConsole::warning("Invalid light entry (not an object), skipping", "VTT");
                skippedCount++;
                continue;
            }

            QJsonObject lightObj = lightValue.toObject();
            LightSource light;

            // Debug: Show all available fields for the first few lights
            if (lightCount < 3) {
                DebugConsole::vtt(QString("Light %1 fields: %2").arg(lightCount).arg(lightObj.keys().join(", ")), "VTT");
            }

            // FLEXIBLE POSITION PARSING: Try multiple possible field structures
            bool hasPosition = false;

            // Option 1: Direct x/y fields
            if (lightObj.contains("x") && lightObj.contains("y")) {
                light.position.setX(lightObj["x"].toDouble());
                light.position.setY(lightObj["y"].toDouble());
                hasPosition = true;
            }
            // Option 2: Nested position object with x/y
            else if (lightObj.contains("position") && lightObj["position"].isObject()) {
                QJsonObject posObj = lightObj["position"].toObject();
                if (posObj.contains("x") && posObj.contains("y")) {
                    light.position.setX(posObj["x"].toDouble());
                    light.position.setY(posObj["y"].toDouble());
                    hasPosition = true;
                }
            }
            // Option 3: Try common VTT field variations
            else if (lightObj.contains("pos") && lightObj["pos"].isObject()) {
                QJsonObject posObj = lightObj["pos"].toObject();
                if (posObj.contains("x") && posObj.contains("y")) {
                    light.position.setX(posObj["x"].toDouble());
                    light.position.setY(posObj["y"].toDouble());
                    hasPosition = true;
                }
            }

            if (!hasPosition) {
                if (lightCount < 5) {
                    DebugConsole::warning(QString("Light %1 missing position, available fields: %2").arg(lightCount).arg(lightObj.keys().join(", ")), "VTT");
                }
                skippedCount++;
                continue;
            }

            // FLEXIBLE RADIUS PARSING: Use defaults if fields are missing
            // Try multiple field name variations common in VTT files
            double dimRadius = 0.0;
            double brightRadius = 0.0;

            // Check for "dim" field variations
            if (lightObj.contains("dim")) {
                dimRadius = qBound(0.0, lightObj["dim"].toDouble(), 10000.0);
            } else if (lightObj.contains("dimRadius")) {
                dimRadius = qBound(0.0, lightObj["dimRadius"].toDouble(), 10000.0);
            } else if (lightObj.contains("dimLight")) {
                dimRadius = qBound(0.0, lightObj["dimLight"].toDouble(), 10000.0);
            }

            // Check for "bright" field variations
            if (lightObj.contains("bright")) {
                brightRadius = qBound(0.0, lightObj["bright"].toDouble(), 10000.0);
            } else if (lightObj.contains("brightRadius")) {
                brightRadius = qBound(0.0, lightObj["brightRadius"].toDouble(), 10000.0);
            } else if (lightObj.contains("brightLight")) {
                brightRadius = qBound(0.0, lightObj["brightLight"].toDouble(), 10000.0);
            } else if (lightObj.contains("range")) {
                // CRITICAL FIX: "range" is in grid squares, must convert to pixels!
                double rangeInSquares = lightObj["range"].toDouble();
                // If we have pixelsPerGrid, use it; otherwise assume 50px per grid
                double pixelScale = (data.pixelsPerGrid > 0) ? data.pixelsPerGrid : 50.0;
                brightRadius = qBound(0.0, rangeInSquares * pixelScale, 10000.0);
                if (lightCount < 3) {
                    DebugConsole::vtt(QString("Light %1 range: %2 grid squares = %3 pixels (scale: %4)").arg(lightCount).arg(rangeInSquares).arg(brightRadius).arg(pixelScale), "VTT");
                }
            } else if (lightObj.contains("radius")) {
                // Some VTT formats use "radius" - assume it's in grid squares too
                double radiusInSquares = lightObj["radius"].toDouble();
                double pixelScale = (data.pixelsPerGrid > 0) ? data.pixelsPerGrid : 50.0;
                brightRadius = qBound(0.0, radiusInSquares * pixelScale, 10000.0);
            }

            // Handle intensity field if present (affects both dim and bright)
            double intensity = 1.0;
            if (lightObj.contains("intensity")) {
                intensity = qBound(0.1, lightObj["intensity"].toDouble(), 10.0);
                if (lightCount < 3) {
                    DebugConsole::vtt(QString("Light %1 has intensity: %2").arg(lightCount).arg(intensity), "VTT");
                }
            }

            // If we only have brightRadius, create dimRadius as 2x bright (standard D&D pattern)
            if (brightRadius > 0 && dimRadius <= 0) {
                dimRadius = brightRadius * 2.0;
            }

            // Apply intensity scaling to both radii
            if (intensity != 1.0) {
                brightRadius *= intensity;
                dimRadius *= intensity;
            }

            // If no radius fields found, use a default visible light
            if (dimRadius <= 0.0 && brightRadius <= 0.0) {
                double pixelScale = (data.pixelsPerGrid > 0) ? data.pixelsPerGrid : 50.0;
                brightRadius = pixelScale * 2.0;  // Default to 2 grid squares bright
                dimRadius = pixelScale * 4.0;     // Default to 4 grid squares dim
                if (lightCount < 3) {
                    DebugConsole::vtt(QString("Light %1 using default radius (2/4 squares)").arg(lightCount), "VTT");
                }
            }

            light.dimRadius = dimRadius;
            light.brightRadius = brightRadius;
            light.intensity = intensity;  // Store the actual intensity value

            // Store shadows flag if present (for future use)
            if (lightObj.contains("shadows")) {
                bool shadows = lightObj["shadows"].toBool();
                // Currently we don't use shadows, but log it for debugging
                if (lightCount < 3 && shadows) {
                    DebugConsole::vtt(QString("Light %1 has shadows enabled").arg(lightCount), "VTT");
                }
            }

            // Parse optional fields with multiple name variations
            light.tintAlpha = 1.0;  // Default to full opacity
            if (lightObj.contains("tintAlpha")) {
                light.tintAlpha = qBound(0.0, lightObj["tintAlpha"].toDouble(), 1.0);
            } else if (lightObj.contains("alpha")) {
                light.tintAlpha = qBound(0.0, lightObj["alpha"].toDouble(), 1.0);
            } else if (lightObj.contains("opacity")) {
                light.tintAlpha = qBound(0.0, lightObj["opacity"].toDouble(), 1.0);
            }

            // Parse color with multiple field variations
            light.tintColor = Qt::white;  // Default to white light
            if (lightObj.contains("tintColor") && lightObj["tintColor"].isString()) {
                light.tintColor = parseHexColor(lightObj["tintColor"].toString());
            } else if (lightObj.contains("color") && lightObj["color"].isString()) {
                light.tintColor = parseHexColor(lightObj["color"].toString());
            } else if (lightObj.contains("colorTint") && lightObj["colorTint"].isString()) {
                light.tintColor = parseHexColor(lightObj["colorTint"].toString());
            }

            data.lights.append(light);
            lightCount++;
        }

        DebugConsole::vtt(QString("Successfully loaded %1 lights, %2 skipped").arg(lightCount).arg(skippedCount), "VTT");
    }

    // Extract and decode the base64 image (support common field variants)
    QString base64Image;
    if (root.contains("image") && root["image"].isString()) {
        base64Image = root["image"].toString();
    } else if (root.contains("image_data") && root["image_data"].isString()) {
        base64Image = root["image_data"].toString();
    } else if (root.contains("map") && root["map"].isString()) {
        base64Image = root["map"].toString();
    } else if (root.contains("mapImage") && root["mapImage"].isString()) {
        base64Image = root["mapImage"].toString();
    }

    if (!base64Image.isEmpty()) {
        DebugConsole::vtt(QString("Found image data, length: %1 characters").arg(base64Image.length()), "VTT");
        reportProgress(60, "Decoding embedded image...");
        data.mapImage = decodeBase64Image(base64Image, progressCallback);

        if (data.mapImage.isNull()) {
            data.errorMessage = "Failed to decode embedded image";
            DebugConsole::error(data.errorMessage, "VTT");
            reportProgress(100, "Image decoding failed");
            return data;
        } else {
            DebugConsole::vtt(QString("Successfully decoded image, size: %1x%2").arg(data.mapImage.width()).arg(data.mapImage.height()), "VTT");
            reportProgress(85, "Image decoded successfully");
        }
    } else {
        data.errorMessage = "No image data found in VTT file";
        DebugConsole::error(data.errorMessage, "VTT");
        reportProgress(100, "No image data found");
        return data;
    }

    // Apply ambient light tint to the image
    if (data.ambientLight != Qt::white && !data.mapImage.isNull()) {
        reportProgress(87, "Applying ambient lighting...");
        DebugConsole::vtt("Starting ambient light tinting...", "VTT");
        DebugConsole::vtt(QString("Ambient color: %1").arg(data.ambientLight.name()), "VTT");
        DebugConsole::vtt(QString("Image size: %1x%2").arg(data.mapImage.width()).arg(data.mapImage.height()), "VTT");
        DebugConsole::vtt(QString("Progress callback: %1").arg(progressCallback ? "YES" : "NO"), "VTT");

        // OPTIMIZATION: Skip tinting if color is very close to white
        // This handles cases like "ffffffff" which don't need processing
        int tintR = data.ambientLight.red();
        int tintG = data.ambientLight.green();
        int tintB = data.ambientLight.blue();

        // If all channels are 254-255 (nearly white), skip tinting entirely
        if (tintR >= 254 && tintG >= 254 && tintB >= 254) {
            DebugConsole::vtt("Skipping ambient light tinting (color is white/near-white)", "VTT");
            reportProgress(95, "Ambient lighting skipped (white)");
        } else {
            // Use direct pixel manipulation for better performance
            QImage tintedImage = data.mapImage.convertToFormat(QImage::Format_ARGB32);

            // Direct pixel access is much faster than pixelColor/setPixelColor
            int width = tintedImage.width();
            int height = tintedImage.height();
            int totalRows = height;
            int progressUpdateInterval = qMax(1, totalRows / 20); // Update progress 20 times

            DebugConsole::vtt(QString("Applying ambient tint to %1x%2 image (%3 pixels)").arg(width).arg(height).arg(width * height), "VTT");

            // Debug first iteration
            bool debugged = false;

            for (int y = 0; y < height; ++y) {
                if (!debugged && y == 0) {
                    DebugConsole::vtt(QString("Starting tint loop, height = %1").arg(height), "VTT");
                    debugged = true;
                }
                
                QRgb* line = reinterpret_cast<QRgb*>(tintedImage.scanLine(y));
                for (int x = 0; x < width; ++x) {
                    QRgb pixel = line[x];
                    int r = (qRed(pixel) * tintR) / 255;
                    int g = (qGreen(pixel) * tintG) / 255;
                    int b = (qBlue(pixel) * tintB) / 255;
                    line[x] = qRgba(r, g, b, qAlpha(pixel));
                }

                // Update progress and keep UI responsive
                if (y % progressUpdateInterval == 0) {
                    int tintProgress = 87 + (8 * y / totalRows); // Progress from 87% to 95%
                    reportProgress(tintProgress, QString("Applying ambient lighting... %1%").arg((100 * y) / totalRows));

                    // The progress callback now handles processEvents safely
                    // We don't call processEvents here to avoid potential hangs
                    // when DD2VTT files are loaded as the first file
                }
            }

            data.mapImage = tintedImage;
            reportProgress(95, "Applied ambient lighting");
        }
    } else if (!data.mapImage.isNull()) {
        // No tinting needed
        DebugConsole::vtt("Ambient lighting skipped (white color or null image)", "VTT");
        reportProgress(95, "Ambient lighting skipped");
    }

    data.isValid = true;
    reportProgress(100, "VTT file loaded successfully");
    return data;
}

bool VTTLoader::isVTTFile(const QString& filepath)
{
    return filepath.endsWith(".dd2vtt", Qt::CaseInsensitive) ||
           filepath.endsWith(".uvtt", Qt::CaseInsensitive) ||
           filepath.endsWith(".df2vtt", Qt::CaseInsensitive);
}

QImage VTTLoader::decodeBase64Image(const QString& base64Data, ProgressCallback progressCallback)
{
    // Helper to report progress safely
    auto reportProgress = [&](int percentage, const QString& message) {
        if (progressCallback) {
            progressCallback(percentage, message);
        }
    };

    reportProgress(62, "Preparing base64 data...");

    DebugConsole::vtt(QString("Raw base64 input length: %1").arg(base64Data.length()), "VTT");
    DebugConsole::vtt(QString("First 50 chars of raw input: %1").arg(base64Data.left(50)), "VTT");

    // VTT files might have data URL prefix, remove it if present
    QString cleanBase64 = base64Data;
    if (cleanBase64.startsWith("data:image/")) {
        int commaIndex = cleanBase64.indexOf(',');
        if (commaIndex != -1) {
            DebugConsole::vtt(QString("Removed data URL prefix, comma at position %1").arg(commaIndex), "VTT");
            cleanBase64 = cleanBase64.mid(commaIndex + 1);
        }
    }

    DebugConsole::vtt(QString("Clean base64 length: %1").arg(cleanBase64.length()), "VTT");
    DebugConsole::vtt(QString("First 50 chars of clean input: %1").arg(cleanBase64.left(50)), "VTT");

    reportProgress(65, "Decoding base64 data...");

    // Check for invalid base64 characters before decoding
    QString base64Pattern = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    bool hasInvalidChars = false;
    for (int i = 0; i < cleanBase64.length(); ++i) {
        if (!base64Pattern.contains(cleanBase64[i]) && !cleanBase64[i].isSpace()) {
            hasInvalidChars = true;
            DebugConsole::warning(QString("Invalid base64 character '%1' at position %2").arg(cleanBase64[i]).arg(i), "VTT");
            if (i > 5) break; // Don't spam too many warnings
        }
    }

    if (hasInvalidChars) {
        DebugConsole::warning("Base64 data contains invalid characters, attempting to clean", "VTT");
        // Remove whitespace and newlines which are common in VTT files
        cleanBase64 = cleanBase64.replace(QRegularExpression("\\s"), "");
        DebugConsole::vtt(QString("Cleaned base64 length after removing whitespace: %1").arg(cleanBase64.length()), "VTT");
    }

    // Use Qt's AbortOnBase64DecodingErrors to ensure strict decoding
    QByteArray imageData = QByteArray::fromBase64(cleanBase64.toUtf8(),
                                                   QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);

    if (imageData.isEmpty()) {
        DebugConsole::error("Base64 decoding resulted in empty data", "VTT");
        DebugConsole::error(QString("Input base64 length: %1").arg(cleanBase64.length()), "VTT");
        DebugConsole::vtt(QString("First 100 chars of base64: %1").arg(cleanBase64.left(100)), "VTT");
        reportProgress(100, "Base64 decoding failed");
        return QImage();
    }

    DebugConsole::vtt(QString("Decoded base64 to %1 bytes").arg(imageData.size()), "VTT");
    reportProgress(70, "Analyzing image format...");

    // Check for common image signatures
    bool isJPEG = false, isPNG = false;
    if (imageData.size() >= 4) {
        QByteArray header = imageData.left(4);
        isJPEG = (imageData[0] == '\xFF' && imageData[1] == '\xD8');
        isPNG = (header == QByteArray("\x89PNG", 4));
        QString formatStr = isJPEG ? "(JPEG)" : isPNG ? "(PNG)" : "(Unknown)";
        DebugConsole::vtt(QString("Image header: %1 %2").arg(QString(header.toHex())).arg(formatStr), "VTT");
    }

    reportProgress(75, "Loading image data...");

    // Try to detect image format from data
    QImage image;
    if (!image.loadFromData(imageData)) {
        DebugConsole::error(QString("Failed to load image from base64 data, size: %1 bytes").arg(imageData.size()), "VTT");

        // Try explicitly with JPEG format first (most common for VTT)
        if (isJPEG && !image.loadFromData(imageData, "JPEG")) {
            DebugConsole::error("Failed to load as JPEG despite valid JPEG header", "VTT");

            // Check if Qt has JPEG support
            QList<QByteArray> formats = QImageReader::supportedImageFormats();
            if (!formats.contains("jpeg") && !formats.contains("jpg")) {
                DebugConsole::error("CRITICAL: Qt was built without JPEG support!", "VTT");
                QStringList formatList;
                for (const QByteArray &format : formats) {
                    formatList << QString::fromLatin1(format);
                }
                DebugConsole::error(QString("Supported formats: %1").arg(formatList.join(", ")), "VTT");
            }
        }

        // Try PNG as fallback
        if (isPNG && !image.loadFromData(imageData, "PNG")) {
            DebugConsole::error("Failed to load as PNG despite valid PNG header", "VTT");
        }

        if (image.isNull()) {
            DebugConsole::error("All image format attempts failed", "VTT");
            return QImage();
        }
    }

    DebugConsole::vtt(QString("Successfully loaded image: %1x%2 format: %3").arg(image.width()).arg(image.height()).arg(image.format()), "VTT");
    reportProgress(80, "Image loading complete");
    return image;
}

QColor VTTLoader::parseHexColor(const QString& hexColor)
{
    // Handle both RRGGBB and RRGGBBAA formats
    QString hex = hexColor;
    if (hex.startsWith("#")) {
        hex = hex.mid(1);
    }

    if (hex.length() == 6) {
        // RRGGBB format
        bool ok;
        int r = hex.mid(0, 2).toInt(&ok, 16);
        if (!ok) return Qt::white;
        int g = hex.mid(2, 2).toInt(&ok, 16);
        if (!ok) return Qt::white;
        int b = hex.mid(4, 2).toInt(&ok, 16);
        if (!ok) return Qt::white;
        return QColor(r, g, b);
    } else if (hex.length() == 8) {
        // CRITICAL FIX: "ffffffff" should always be white
        // DD2VTT uses this for no tinting
        if (hex.toLower() == "ffffffff") {
            return Qt::white;
        }

        // Could be RRGGBBAA or AARRGGBB format
        // For DD2VTT files, ambient_light is typically RRGGBBAA
        // But some formats use AARRGGBB
        bool ok;

        // Try RRGGBBAA format first (most common for ambient_light)
        int r = hex.mid(0, 2).toInt(&ok, 16);
        if (!ok) return Qt::white;
        int g = hex.mid(2, 2).toInt(&ok, 16);
        if (!ok) return Qt::white;
        int b = hex.mid(4, 2).toInt(&ok, 16);
        if (!ok) return Qt::white;
        int a = hex.mid(6, 2).toInt(&ok, 16);
        if (!ok) return Qt::white;

        // For ambient light, we only care about RGB, not alpha
        return QColor(r, g, b);
    }

    // Default to white if parsing fails
    return Qt::white;
}
