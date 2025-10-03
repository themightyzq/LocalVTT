#ifndef SECUREWINDOWREGISTRY_H
#define SECUREWINDOWREGISTRY_H

#include <QObject>
#include <QWidget>
#include <QSet>
#include <QMutex>
#include <QCryptographicHash>
#include <QRandomGenerator>

class SecureWindowRegistry : public QObject
{
    Q_OBJECT
public:
    static SecureWindowRegistry& instance();

    enum WindowType {
        Invalid = 0,
        MainWindow = 1,
        PlayerWindow = 2
    };

    QString registerWindow(QWidget* window, WindowType type);
    void unregisterWindow(QWidget* window);
    WindowType getWindowType(QWidget* window) const;
    bool isDMWindow(QWidget* window) const;
    bool isPlayerWindow(QWidget* window) const;

private:
    SecureWindowRegistry() = default;
    ~SecureWindowRegistry() = default;
    SecureWindowRegistry(const SecureWindowRegistry&) = delete;
    SecureWindowRegistry& operator=(const SecureWindowRegistry&) = delete;

    QString generateSecureToken(QWidget* window, WindowType type) const;
    bool validateToken(QWidget* window, const QString& token, WindowType expectedType) const;

    struct WindowInfo {
        QWidget* widget;
        WindowType type;
        QString secureToken;
        qint64 registrationTime;
    };

    mutable QMutex m_mutex;
    QHash<QWidget*, WindowInfo> m_registeredWindows;
    QHash<QString, QWidget*> m_tokenToWindow;
};

#endif // SECUREWINDOWREGISTRY_H