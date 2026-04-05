#ifndef CUSTOMPRESETMANAGER_H
#define CUSTOMPRESETMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include "controllers/AtmospherePreset.h"

// Singleton manager for custom atmosphere presets
// Handles saving/loading presets to JSON files in AppDataLocation/presets/
class CustomPresetManager : public QObject
{
    Q_OBJECT

public:
    static CustomPresetManager& instance();

    // Save current state as a new custom preset
    bool savePreset(const QString& name, const QString& description,
                    const AtmosphereState& state);

    // Delete a custom preset by name
    bool deletePreset(const QString& name);

    // Get all custom presets (loaded from disk)
    QList<AtmospherePreset> getCustomPresets() const;

    // Check if a preset name already exists (custom or built-in)
    bool presetExists(const QString& name) const;

    // Check if a preset is custom (vs built-in)
    bool isCustomPreset(const QString& name) const;

    // Get the presets directory path
    QString getPresetsDirectory() const;

    // Reload presets from disk
    void refresh();

signals:
    void presetsChanged();  // Emitted when presets are saved/deleted

private:
    CustomPresetManager();
    ~CustomPresetManager() = default;
    CustomPresetManager(const CustomPresetManager&) = delete;
    CustomPresetManager& operator=(const CustomPresetManager&) = delete;

    void loadPresetsFromDisk();
    QString presetFilePath(const QString& name) const;
    QString sanitizeFileName(const QString& name) const;

    QList<AtmospherePreset> m_customPresets;
    bool m_loaded = false;
};

#endif // CUSTOMPRESETMANAGER_H
