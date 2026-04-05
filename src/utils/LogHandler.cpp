#include "utils/LogHandler.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <cstdio>
#include <csignal>

static FILE* s_logFile = nullptr;
static QString s_logPath;
static QtMessageHandler s_defaultHandler = nullptr;
static constexpr qint64 MAX_LOG_SIZE = 5 * 1024 * 1024; // 5MB

static void rotateLogIfNeeded()
{
    QFileInfo fi(s_logPath);
    if (fi.exists() && fi.size() > MAX_LOG_SIZE) {
        QString backupPath = s_logPath + ".1";
        QFile::remove(backupPath);
        QFile::rename(s_logPath, backupPath);
    }
}

static const char* msgTypeString(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return "DEBUG";
    case QtInfoMsg:     return "INFO ";
    case QtWarningMsg:  return "WARN ";
    case QtCriticalMsg: return "CRIT ";
    case QtFatalMsg:    return "FATAL";
    }
    return "?????";
}

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    // Write to log file
    if (s_logFile) {
        QByteArray timestamp = QDateTime::currentDateTime()
            .toString("yyyy-MM-dd hh:mm:ss.zzz").toUtf8();
        QByteArray message = msg.toUtf8();
        fprintf(s_logFile, "[%s] %s: %s\n",
                timestamp.constData(), msgTypeString(type), message.constData());
        fflush(s_logFile);
    }

    // Pass through to default handler (console output during development)
    if (s_defaultHandler) {
        s_defaultHandler(type, context, msg);
    }
}

static void crashHandler(int sig)
{
    // Minimal signal handler — no Qt calls, just raw I/O
    const char* sigName = "UNKNOWN";
    switch (sig) {
    case SIGSEGV: sigName = "SIGSEGV"; break;
    case SIGABRT: sigName = "SIGABRT"; break;
#ifdef SIGBUS
    case SIGBUS:  sigName = "SIGBUS";  break;
#endif
    }

    if (s_logFile) {
        fprintf(s_logFile, "[CRASH] Fatal signal: %s (%d)\n", sigName, sig);
        fflush(s_logFile);
        fclose(s_logFile);
        s_logFile = nullptr;
    }
    fprintf(stderr, "FATAL: Signal %s (%d) received\n", sigName, sig);

    // Re-raise to get default crash behavior (core dump, crash report)
    signal(sig, SIG_DFL);
    raise(sig);
}

void LogHandler::install(const QString& logDir)
{
    QString dir = logDir;
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    }
    QDir().mkpath(dir);

    s_logPath = dir + "/projectvtt.log";
    rotateLogIfNeeded();

    s_logFile = fopen(s_logPath.toUtf8().constData(), "a");
    if (s_logFile) {
        fprintf(s_logFile, "\n=== Project VTT started at %s ===\n",
                QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toUtf8().constData());
        fflush(s_logFile);
    }

    // Install Qt message handler
    s_defaultHandler = qInstallMessageHandler(messageHandler);

    // Install crash signal handlers
    signal(SIGSEGV, crashHandler);
    signal(SIGABRT, crashHandler);
#ifdef SIGBUS
    signal(SIGBUS, crashHandler);
#endif
}
