#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QString>
#include <QObject>
#include <QGlobalStatic>
#include <functional>

enum class ErrorLevel {
    Info,
    Warning,
    Error,
    Critical
};

class ErrorHandler : public QObject
{
    Q_OBJECT

public:
    static ErrorHandler& instance();

    // Report errors with user notification
    void reportError(const QString& message, ErrorLevel level = ErrorLevel::Error);
    void reportErrorWithRecovery(const QString& message,
                                std::function<void()> recoveryAction,
                                ErrorLevel level = ErrorLevel::Error);

    // Set callback for displaying errors to user
    void setErrorCallback(std::function<void(const QString&, ErrorLevel)> callback);

    // Log error to file for debugging
    void logError(const QString& context, const QString& error);

    // Q_GLOBAL_STATIC requires public constructor/destructor
    ErrorHandler() = default;
    ~ErrorHandler() = default;

signals:
    void errorOccurred(const QString& message, ErrorLevel level);

private:
    std::function<void(const QString&, ErrorLevel)> m_errorCallback;

    QString errorLevelToString(ErrorLevel level) const;
};

#endif // ERRORHANDLER_H