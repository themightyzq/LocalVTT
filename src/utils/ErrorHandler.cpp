#include "utils/ErrorHandler.h"
#include "utils/DebugConsole.h"
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QGlobalStatic>

// Use Q_GLOBAL_STATIC for thread-safe, lazy initialization without recursion issues
Q_GLOBAL_STATIC(ErrorHandler, g_errorHandlerInstance)

ErrorHandler& ErrorHandler::instance()
{
    return *g_errorHandlerInstance();
}

void ErrorHandler::reportError(const QString& message, ErrorLevel level)
{
    // Log to debug output via DebugConsole
    switch (level) {
    case ErrorLevel::Info:
        DebugConsole::info(message, "Error");
        break;
    case ErrorLevel::Warning:
        DebugConsole::warning(message, "Error");
        break;
    case ErrorLevel::Error:
        DebugConsole::error(message, "Error");
        break;
    case ErrorLevel::Critical:
        DebugConsole::error(QString("CRITICAL: %1").arg(message), "Error");
        break;
    }

    // Log to file
    logError(errorLevelToString(level), message);

    // Notify UI if callback is set
    if (m_errorCallback) {
        m_errorCallback(message, level);
    }

    // Emit signal for any connected listeners
    emit errorOccurred(message, level);
}

void ErrorHandler::reportErrorWithRecovery(const QString& message,
                                          std::function<void()> recoveryAction,
                                          ErrorLevel level)
{
    reportError(message, level);

    // Attempt recovery if provided
    if (recoveryAction) {
        try {
            recoveryAction();
            reportError("Recovery attempted for: " + message, ErrorLevel::Info);
        } catch (...) {
            reportError("Recovery failed for: " + message, ErrorLevel::Critical);
        }
    }
}

void ErrorHandler::setErrorCallback(std::function<void(const QString&, ErrorLevel)> callback)
{
    m_errorCallback = callback;
}

void ErrorHandler::logError(const QString& context, const QString& error)
{
    // Get log file path
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    QString logPath = logDir + "/projectvtt_errors.log";

    QFile logFile(logPath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
               << " [" << context << "] " << error << Qt::endl;
    }
}

QString ErrorHandler::errorLevelToString(ErrorLevel level) const
{
    switch (level) {
    case ErrorLevel::Info:
        return "INFO";
    case ErrorLevel::Warning:
        return "WARNING";
    case ErrorLevel::Error:
        return "ERROR";
    case ErrorLevel::Critical:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}