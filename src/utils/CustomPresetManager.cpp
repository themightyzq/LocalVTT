#include "CustomPresetManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>

CustomPresetManager& CustomPresetManager::instance()
{
    static CustomPresetManager instance;
    return instance;
}

CustomPresetManager::CustomPresetManager()
    : QObject(nullptr)
{
    // Ensure presets directory exists
    QDir().mkpath(getPresetsDirectory());
}

QString CustomPresetManager::getPresetsDirectory() const
{
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData + "/presets";
}

QString CustomPresetManager::sanitizeFileName(const QString& name) const
{
    QString sanitized = name;
    // Replace characters that are problematic in filenames
    sanitized.replace('/', '_');
    sanitized.replace('\\', '_');
    sanitized.replace(':', '_');
    sanitized.replace('*', '_');
    sanitized.replace('?', '_');
    sanitized.replace('"', '_');
    sanitized.replace('<', '_');
    sanitized.replace('>', '_');
    sanitized.replace('|', '_');
    return sanitized.trimmed();
}

QString CustomPresetManager::presetFilePath(const QString& name) const
{
    return getPresetsDirectory() + "/" + sanitizeFileName(name) + ".json";
}

void CustomPresetManager::loadPresetsFromDisk()
{
    m_customPresets.clear();

    QDir presetsDir(getPresetsDirectory());
    if (!presetsDir.exists()) {
        m_loaded = true;
        return;
    }

    QStringList jsonFiles = presetsDir.entryList(QStringList() << "*.json", QDir::Files);

    for (const QString& fileName : jsonFiles) {
        QString filePath = presetsDir.absoluteFilePath(fileName);
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "CustomPresetManager: Failed to open" << filePath;
            continue;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            qWarning() << "CustomPresetManager: Invalid JSON in" << filePath;
            continue;
        }

        QJsonObject root = doc.object();

        // Check version for future compatibility
        int version = root["version"].toInt(1);
        if (version > 1) {
            qWarning() << "CustomPresetManager: Unsupported version" << version << "in" << filePath;
            // Still try to load - may work with backward compatibility
        }

        AtmospherePreset preset;
        preset.name = root["name"].toString();
        preset.description = root["description"].toString();

        if (root.contains("state") && root["state"].isObject()) {
            preset.state = AtmosphereState::fromJson(root["state"].toObject());
        }

        if (preset.name.isEmpty()) {
            // Use filename as fallback
            preset.name = fileName.left(fileName.length() - 5);  // Remove .json
        }

        m_customPresets.append(preset);
        qDebug() << "CustomPresetManager: Loaded preset" << preset.name;
    }

    m_loaded = true;
    qDebug() << "CustomPresetManager: Loaded" << m_customPresets.size() << "custom presets";
}

QList<AtmospherePreset> CustomPresetManager::getCustomPresets() const
{
    // Lazy load on first access
    if (!m_loaded) {
        const_cast<CustomPresetManager*>(this)->loadPresetsFromDisk();
    }
    return m_customPresets;
}

bool CustomPresetManager::presetExists(const QString& name) const
{
    // Check built-in presets
    QList<AtmospherePreset> builtIn = AtmospherePreset::getBuiltInPresets();
    for (const auto& preset : builtIn) {
        if (preset.name.compare(name, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    // Check custom presets
    return isCustomPreset(name);
}

bool CustomPresetManager::isCustomPreset(const QString& name) const
{
    QList<AtmospherePreset> custom = getCustomPresets();
    for (const auto& preset : custom) {
        if (preset.name.compare(name, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool CustomPresetManager::savePreset(const QString& name, const QString& description,
                                      const AtmosphereState& state)
{
    if (name.trimmed().isEmpty()) {
        qWarning() << "CustomPresetManager: Cannot save preset with empty name";
        return false;
    }

    // Limit custom preset count to prevent unbounded disk usage
    if (m_customPresets.size() >= 100) {
        qWarning() << "CustomPresetManager: Maximum 100 custom presets reached";
        return false;
    }

    // Build JSON document
    QJsonObject root;
    root["version"] = 1;
    root["name"] = name.trimmed();
    root["description"] = description;
    root["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    root["state"] = state.toJson();

    QJsonDocument doc(root);

    // Ensure directory exists
    QDir().mkpath(getPresetsDirectory());

    // Write to file
    QString filePath = presetFilePath(name);
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "CustomPresetManager: Failed to write" << filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "CustomPresetManager: Saved preset" << name << "to" << filePath;

    // Reload presets and notify
    loadPresetsFromDisk();
    emit presetsChanged();

    return true;
}

bool CustomPresetManager::deletePreset(const QString& name)
{
    QString filePath = presetFilePath(name);
    QFile file(filePath);

    if (!file.exists()) {
        qWarning() << "CustomPresetManager: Preset file not found:" << filePath;
        return false;
    }

    if (!file.remove()) {
        qWarning() << "CustomPresetManager: Failed to delete" << filePath;
        return false;
    }

    qDebug() << "CustomPresetManager: Deleted preset" << name;

    // Reload presets and notify
    loadPresetsFromDisk();
    emit presetsChanged();

    return true;
}

void CustomPresetManager::refresh()
{
    loadPresetsFromDisk();
    emit presetsChanged();
}
