#ifndef ACTIONREGISTRY_H
#define ACTIONREGISTRY_H

#include <QObject>
#include <QAction>
#include <QKeySequence>
#include <QHash>
#include <QStringList>

enum class ActionCategory
{
    Tools,
    Fog,
    View,
    Privacy,
    Player,
    File,
    Edit,
    Grid,
    Lighting,
    Debug,
    System
};

struct ActionInfo
{
    QString id;
    QString text;
    QString tooltip;
    QKeySequence shortcut;
    ActionCategory category;
    bool checkable;
    QString statusTip;
};

class ActionRegistry : public QObject
{
    Q_OBJECT

public:
    explicit ActionRegistry(QObject* parent = nullptr);
    ~ActionRegistry() = default;
    
    QAction* getAction(const QString& actionId);
    QAction* createAction(const QString& actionId, QObject* parent = nullptr);
    
    bool hasAction(const QString& actionId) const;
    QKeySequence getShortcut(const QString& actionId) const;
    QString getHelpText(const QString& actionId) const;
    QStringList getActionsForCategory(ActionCategory category) const;
    
    bool hasConflict(const QKeySequence& shortcut, const QString& excludeActionId = QString()) const;
    QStringList getConflicts(const QKeySequence& shortcut) const;
    
    QStringList getAllShortcutDescriptions() const;
    QString getShortcutHelpText() const;

private:
    
    void initializeActions();
    void registerAction(const QString& id, const QString& text, const QString& tooltip,
                       const QKeySequence& shortcut, ActionCategory category, 
                       bool checkable = false, const QString& statusTip = QString());
    
    QHash<QString, ActionInfo> m_actionInfos;
    QHash<QString, QAction*> m_actions;
    

};

#endif // ACTIONREGISTRY_H