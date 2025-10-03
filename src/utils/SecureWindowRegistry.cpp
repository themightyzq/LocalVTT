#include "SecureWindowRegistry.h"
#include "DebugConsole.h"
#include <QDateTime>
#include <QApplication>

SecureWindowRegistry& SecureWindowRegistry::instance()
{
    static SecureWindowRegistry instance;
    return instance;
}

QString SecureWindowRegistry::registerWindow(QWidget* window, WindowType type)
{
    if (!window || type == Invalid) {
        DebugConsole::error("Invalid window or type in registration", "SecureWindowRegistry");
        return QString();
    }

    QMutexLocker locker(&m_mutex);

    if (m_registeredWindows.contains(window)) {
        DebugConsole::warning("Window already registered - updating", "SecureWindowRegistry");
        unregisterWindow(window);
    }

    QString secureToken = generateSecureToken(window, type);

    WindowInfo info;
    info.widget = window;
    info.type = type;
    info.secureToken = secureToken;
    info.registrationTime = QDateTime::currentMSecsSinceEpoch();

    m_registeredWindows[window] = info;
    m_tokenToWindow[secureToken] = window;

    connect(window, &QWidget::destroyed, this, [this, window]() {
        unregisterWindow(window);
    }, Qt::DirectConnection);

    const char* typeStr = (type == MainWindow) ? "MainWindow" : "PlayerWindow";
    DebugConsole::info(QString("Window registered as %1 (token: %2)")
                       .arg(typeStr)
                       .arg(secureToken.left(16) + "..."), "SecureWindowRegistry");

    return secureToken;
}

void SecureWindowRegistry::unregisterWindow(QWidget* window)
{
    if (!window) return;

    QMutexLocker locker(&m_mutex);

    if (!m_registeredWindows.contains(window)) {
        return;
    }

    const WindowInfo& info = m_registeredWindows[window];
    m_tokenToWindow.remove(info.secureToken);
    m_registeredWindows.remove(window);

    DebugConsole::info("Window unregistered", "SecureWindowRegistry");
}

SecureWindowRegistry::WindowType SecureWindowRegistry::getWindowType(QWidget* window) const
{
    if (!window) return Invalid;

    QWidget* topLevel = window->window();
    if (!topLevel) return Invalid;

    QMutexLocker locker(&m_mutex);

    auto it = m_registeredWindows.constFind(topLevel);
    if (it == m_registeredWindows.constEnd()) {
        return Invalid;
    }

    if (!validateToken(topLevel, it->secureToken, it->type)) {
        DebugConsole::error("Security violation: Token validation failed", "SecureWindowRegistry");
        return Invalid;
    }

    return it->type;
}

bool SecureWindowRegistry::isDMWindow(QWidget* window) const
{
    return getWindowType(window) == MainWindow;
}

bool SecureWindowRegistry::isPlayerWindow(QWidget* window) const
{
    return getWindowType(window) == PlayerWindow;
}

QString SecureWindowRegistry::generateSecureToken(QWidget* window, WindowType type) const
{
    QCryptographicHash hash(QCryptographicHash::Sha256);

    hash.addData(reinterpret_cast<const char*>(&window), sizeof(window));
    hash.addData(reinterpret_cast<const char*>(&type), sizeof(type));

    quint64 random1 = QRandomGenerator::global()->generate64();
    quint64 random2 = QRandomGenerator::global()->generate64();
    hash.addData(reinterpret_cast<const char*>(&random1), sizeof(random1));
    hash.addData(reinterpret_cast<const char*>(&random2), sizeof(random2));

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    hash.addData(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));

    QString appId = QApplication::applicationName() + QApplication::applicationVersion();
    hash.addData(appId.toUtf8());

    return QString::fromLatin1(hash.result().toHex());
}

bool SecureWindowRegistry::validateToken(QWidget* window, const QString& token, WindowType expectedType) const
{
    if (!window || token.isEmpty() || expectedType == Invalid) {
        return false;
    }

    auto it = m_tokenToWindow.constFind(token);
    if (it == m_tokenToWindow.constEnd()) {
        return false;
    }

    if (it.value() != window) {
        return false;
    }

    auto winIt = m_registeredWindows.constFind(window);
    if (winIt == m_registeredWindows.constEnd()) {
        return false;
    }

    if (winIt->type != expectedType) {
        return false;
    }

    if (winIt->secureToken != token) {
        return false;
    }

    return true;
}