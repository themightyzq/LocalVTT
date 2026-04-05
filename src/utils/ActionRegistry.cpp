#include "ActionRegistry.h"
#include <QApplication>
#include <QDebug>

ActionRegistry::ActionRegistry(QObject* parent)
    : QObject(parent)
{
    initializeActions();
}

void ActionRegistry::initializeActions()
{
    // CLAUDE.md 3.6: NO number keys for tool switching
    // Tool actions registered without shortcuts per CLAUDE.md compliance
    registerAction("tool_pan", "Pan/Navigate", "Switch to pan/navigate mode", QKeySequence(), ActionCategory::Tools, false, "Pan and navigate around the map");
    registerAction("tool_fog", "Fog Tool", "Switch to fog of war tool", QKeySequence(), ActionCategory::Tools, false, "Hide and reveal areas of the map");
    registerAction("tool_pointer", "Pointer/Beacon", "Switch to pointer/beacon tool", QKeySequence(), ActionCategory::Tools, false, "Point out locations to players");
    registerAction("tool_escape", "Return to Pan", "Return to pan/navigate mode", QKeySequence("Escape"), ActionCategory::Tools, false, "Return to default navigation mode");

    // Simplified fog actions per CLAUDE.md - single tool with modifiers
    registerAction("fog_toggle", "Toggle Fog of War", "Toggle fog of war display", QKeySequence("F"), ActionCategory::Fog, true, "Enable or disable fog of war");
    registerAction("fog_brush_smaller", "Decrease Brush Size", "Make fog brush smaller", QKeySequence("["), ActionCategory::Fog, false, "Decrease the size of the fog brush");
    registerAction("fog_brush_larger", "Increase Brush Size", "Make fog brush larger", QKeySequence("]"), ActionCategory::Fog, false, "Increase the size of the fog brush");
    registerAction("fog_clear", "Clear Fog of War", "Clear all fog of war", QKeySequence("Ctrl+Shift+F"), ActionCategory::Fog, false, "Remove all fog of war from the map");
    registerAction("fog_reset", "Reset Fog of War", "Reset fog to cover entire map", QKeySequence("Ctrl+Shift+R"), ActionCategory::Fog, false, "Reset fog to cover the entire map");
    registerAction("fog_undo", "Undo Fog Change", "Undo last fog operation", QKeySequence::Undo, ActionCategory::Fog, false, "Undo the last fog of war change");
    registerAction("fog_redo", "Redo Fog Change", "Redo last fog operation", QKeySequence::Redo, ActionCategory::Fog, false, "Redo the last undone fog of war change");
    registerAction("fog_hide_toggle", "Toggle Fog Hide Mode", "Switch between reveal and hide", QKeySequence("H"), ActionCategory::Fog, true, "Toggle fog hide/reveal mode");

    // Note: Fit to Screen shortcut "/" is set in MenuManager to avoid ambiguity
    registerAction("view_fit_screen", "Fit to Screen", "Fit map to screen", QKeySequence("/"), ActionCategory::View, false, "Fit the entire map to the screen");
    registerAction("view_zoom_in", "Zoom In", "Zoom in on map", QKeySequence::ZoomIn, ActionCategory::View, false, "Zoom in on the map");
    registerAction("view_zoom_out", "Zoom Out", "Zoom out from map", QKeySequence::ZoomOut, ActionCategory::View, false, "Zoom out from the map");
    // CLAUDE.md 3.6: Ctrl+1/2/3 zoom presets
    registerAction("view_zoom_100", "Zoom 100%", "Zoom to 100%", QKeySequence("Ctrl+1"), ActionCategory::View, false, "Set zoom to 100%");
    registerAction("view_zoom_200", "Zoom 200%", "Zoom to 200%", QKeySequence("Ctrl+2"), ActionCategory::View, false, "Set zoom to 200%");
    registerAction("view_zoom_300", "Zoom 300%", "Zoom to 300%", QKeySequence("Ctrl+3"), ActionCategory::View, false, "Set zoom to 300%");
    // CLAUDE.md 3.6: B = Map Browser, A = Atmosphere Panel
    registerAction("view_map_browser", "Toggle Map Browser", "Show/hide map browser panel", QKeySequence("B"), ActionCategory::View, false, "Open or close the map browser sidebar");
    registerAction("view_atmosphere", "Toggle Atmosphere Panel", "Show/hide atmosphere controls", QKeySequence("A"), ActionCategory::View, false, "Open or close the atmosphere toolbox");

    // B is for Map Browser per CLAUDE.md 3.6, not blackout
    registerAction("privacy_blackout", "Emergency Blackout", "Instant privacy blackout", QKeySequence(), ActionCategory::Privacy, false, "Immediately black out the player screen");
    registerAction("privacy_intermission", "Intermission Screen", "Show intermission screen", QKeySequence(), ActionCategory::Privacy, false, "Display intermission screen for breaks");

    // CLAUDE.md 3.6: Player Window shortcut is "P" (set in MainWindow::createMenus)
    registerAction("player_window_toggle", "Toggle Player Window", "Show/hide player window", QKeySequence("P"), ActionCategory::Player, false, "Open or close the player display window");
    registerAction("window_player", "Player Window", "Toggle player window", QKeySequence(), ActionCategory::Player, false, "Open or close the TV display window");
    registerAction("player_fullscreen", "Player Fullscreen", "Toggle player window fullscreen", QKeySequence("F11"), ActionCategory::Player, false, "Toggle fullscreen mode for player window");
    // CLAUDE.md 3.6: V = Sync View, Shift+V = Reset Auto-Fit
    registerAction("player_sync", "Sync View to Players", "Push DM view to player window", QKeySequence("V"), ActionCategory::Player, false, "Push current DM view to player window");
    registerAction("player_auto_fit_reset", "Reset Player Auto-Fit", "Reset player window to auto-fit", QKeySequence("Shift+V"), ActionCategory::Player, false, "Reset player window to automatic fit-to-screen");

    registerAction("file_open", "Open Map", "Open map file", QKeySequence::Open, ActionCategory::File, false, "Open a map image or VTT file");
    // Ctrl+S removed - conflicts with expected file save behavior (CLAUDE.md UI consistency)
    registerAction("file_save", "Quick Save", "Quick save fog state", QKeySequence(), ActionCategory::File, false, "Quickly save current fog state");
    // Ctrl+L removed - confusing shortcut (CLAUDE.md UI consistency)
    registerAction("file_load", "Quick Load", "Quick load fog state", QKeySequence(), ActionCategory::File, false, "Quickly load saved fog state");
    // Recent files don't need shortcuts - accessible via menu
    registerAction("file_quit", "Quit", "Exit application", QKeySequence::Quit, ActionCategory::File, false, "Exit Project VTT");

    registerAction("edit_undo", "Undo", "Undo last action", QKeySequence::Undo, ActionCategory::Edit, false, "Undo the last action");
    registerAction("edit_redo", "Redo", "Redo last action", QKeySequence::Redo, ActionCategory::Edit, false, "Redo the last undone action");

    registerAction("grid_toggle", "Toggle Grid", "Show/hide grid overlay", QKeySequence("G"), ActionCategory::Grid, true, "Toggle grid overlay display");
    // Ctrl+I, Ctrl+H, Ctrl+Shift+G removed - use toolbar/menu instead (CLAUDE.md UI consistency)
    registerAction("grid_info", "Grid Information", "Show grid information", QKeySequence(), ActionCategory::Grid, false, "Display current grid settings");
    registerAction("grid_type", "Toggle Grid Type", "Switch between square and hex grid", QKeySequence(), ActionCategory::Grid, false, "Switch between square and hexagonal grid");
    registerAction("grid_calibrate", "Calibrate Grid", "Open grid calibration tool", QKeySequence(), ActionCategory::Grid, false, "Calibrate grid for TV display");

    // Lighting is functional - keep these
    registerAction("lighting_toggle", "Toggle Lighting", "Enable/disable lighting system", QKeySequence("L"), ActionCategory::Lighting, true, "Toggle dynamic lighting system");

    registerAction("debug_console", "Debug Console", "Toggle debug console", QKeySequence("F12"), ActionCategory::Debug, false, "Open or close the debug console");

    // Player view mode is functional
    registerAction("system_player_view", "Toggle Player View Mode", "Toggle DM player view mode", QKeySequence("Ctrl+P"), ActionCategory::System, true, "View map as players see it");

    // Help menu actions - keep these as they're informational
    registerAction("help_shortcuts", "Keyboard Shortcuts", "Show keyboard shortcuts", QKeySequence("F1"), ActionCategory::System, false, "Display keyboard shortcuts reference");
    registerAction("help_about", "About Project VTT", "About this application", QKeySequence(), ActionCategory::System, false, "Show information about Project VTT");
}

void ActionRegistry::registerAction(const QString& id, const QString& text, const QString& tooltip,
                                   const QKeySequence& shortcut, ActionCategory category, 
                                   bool checkable, const QString& statusTip)
{
    ActionInfo info;
    info.id = id;
    info.text = text;
    info.tooltip = tooltip;
    info.shortcut = shortcut;
    info.category = category;
    info.checkable = checkable;
    info.statusTip = statusTip.isEmpty() ? tooltip : statusTip;
    
    m_actionInfos[id] = info;
}

QAction* ActionRegistry::getAction(const QString& actionId)
{
    if (m_actions.contains(actionId)) {
        return m_actions[actionId];
    }
    return nullptr;
}

QAction* ActionRegistry::createAction(const QString& actionId, QObject* parent)
{
    if (!m_actionInfos.contains(actionId)) {
        qWarning() << "ActionRegistry: Unknown action ID:" << actionId;
        return nullptr;
    }
    
    if (m_actions.contains(actionId)) {
        return m_actions[actionId];
    }
    
    const ActionInfo& info = m_actionInfos[actionId];
    QAction* action = new QAction(info.text, parent ? parent : this);
    action->setShortcut(info.shortcut);
    action->setToolTip(info.tooltip);
    action->setStatusTip(info.statusTip);
    action->setCheckable(info.checkable);
    action->setObjectName(actionId);
    
    m_actions[actionId] = action;
    return action;
}

bool ActionRegistry::hasAction(const QString& actionId) const
{
    return m_actionInfos.contains(actionId);
}

QKeySequence ActionRegistry::getShortcut(const QString& actionId) const
{
    if (m_actionInfos.contains(actionId)) {
        return m_actionInfos[actionId].shortcut;
    }
    return QKeySequence();
}

QString ActionRegistry::getHelpText(const QString& actionId) const
{
    if (m_actionInfos.contains(actionId)) {
        const ActionInfo& info = m_actionInfos[actionId];
        return QString("%1 (%2)").arg(info.text).arg(info.shortcut.toString());
    }
    return QString();
}

QStringList ActionRegistry::getActionsForCategory(ActionCategory category) const
{
    QStringList actions;
    for (auto it = m_actionInfos.begin(); it != m_actionInfos.end(); ++it) {
        if (it.value().category == category) {
            actions.append(it.key());
        }
    }
    return actions;
}

bool ActionRegistry::hasConflict(const QKeySequence& shortcut, const QString& excludeActionId) const
{
    if (shortcut.isEmpty()) {
        return false;
    }
    
    for (auto it = m_actionInfos.begin(); it != m_actionInfos.end(); ++it) {
        if (it.key() != excludeActionId && it.value().shortcut == shortcut) {
            return true;
        }
    }
    return false;
}

QStringList ActionRegistry::getConflicts(const QKeySequence& shortcut) const
{
    QStringList conflicts;
    if (shortcut.isEmpty()) {
        return conflicts;
    }
    
    for (auto it = m_actionInfos.begin(); it != m_actionInfos.end(); ++it) {
        if (it.value().shortcut == shortcut) {
            conflicts.append(it.key());
        }
    }
    return conflicts;
}

QStringList ActionRegistry::getAllShortcutDescriptions() const
{
    QStringList descriptions;
    
    QStringList categories = {"Tools", "Fog Operations", "View Controls", "Privacy Shield", 
                             "Player Window", "File Operations", "Edit", "Grid", "Lighting", "System"};
    
    for (int catIndex = 0; catIndex < categories.size(); ++catIndex) {
        ActionCategory category = static_cast<ActionCategory>(catIndex);
        QStringList categoryActions = getActionsForCategory(category);
        
        if (!categoryActions.isEmpty()) {
            descriptions.append(QString("=== %1 ===").arg(categories[catIndex]));
            
            for (const QString& actionId : categoryActions) {
                const ActionInfo& info = m_actionInfos[actionId];
                if (!info.shortcut.isEmpty()) {
                    descriptions.append(QString("%1: %2").arg(info.shortcut.toString()).arg(info.text));
                }
            }
            descriptions.append("");
        }
    }
    
    return descriptions;
}

QString ActionRegistry::getShortcutHelpText() const
{
    QStringList descriptions = getAllShortcutDescriptions();
    return descriptions.join("\n");
}