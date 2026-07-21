#include "logger.h"

#include <QDir>
#include <QtLogging>
#include <QDateTime>
#include <QTextStream>
#include <QDirIterator>

Logger* Logger::s_instance = nullptr;

Logger *Logger::instance()
{
    if(s_instance == nullptr)
    {
        s_instance = new Logger();
    }
    return s_instance;
}

void Logger::init(const QString dir, bool consoleOutput, bool fileOutput)
{
    m_consoleOutput = consoleOutput;
    m_fileOutput = fileOutput;

    QString today = QDateTime::currentDateTime().toString("yyyyMMdd");
    m_logDir = dir + "/" + today;
    QDir().mkpath(m_logDir);

    rotatelog();

    cleanOldLogs();

    qInstallMessageHandler(messageHandler);
}

void Logger::setMinLevel(Logger::LogLevel level)
{
    m_minLevel = level;
}

/*
void Logger::log(Logger::LogLevel level, const QString &message, const char *file, int line)
{
    if(level < m_minLevel) return;

    QString levelStr;
    switch(level)
    {
    case Debug: levelStr = "DEBUG"; break;
    case Info: levelStr = "INFO"; break;
    case Warning: levelStr = "WARNING"; break;
    case Error: levelStr = "Error"; break;
    }

    QString fileName = file ? QString(file).section('/',-1) : "unknown";
    QString logLine = QString("[%1] [%2] [%3:%4] %5")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(levelStr,5)
            .arg(fileName, 20)
            .arg(line,4)
            .arg(message);

    if(m_consoleOutput)
    {
        QTextStream(stdout) << logLine << "\n";
    }

    if(m_fileOutput && m_logFile.isOpen())
    {
        QMutexLocker locker(&m_mutex);
        QTextStream stream(&m_logFile);
        stream << logLine << "\n";

        if(m_logFile.size() > m_maxFileSize)
        {
            locker.unlock();
            rotatelog();
        }
    }

}
*/

QString Logger::currentLogFile() const
{
    return m_currentFile;
}


void Logger::writeLog(const QString &level, const QString &file, int line, const QString &msg)
{
    QString fileName = file.isEmpty() ? "unknown" : file;
    int lastSlash = fileName.lastIndexOf('/');
    int lastBackslash = fileName.lastIndexOf('\\');
    int pos = qMax(lastSlash, lastBackslash);
    if (pos >= 0) fileName = fileName.mid(pos + 1);

    QString logLine = QString("[%1] [%2] [%3:%4] %5")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
            .arg(level, 7)
            .arg(fileName, 25)
            .arg(line, 4)
            .arg(msg);

    // 控制台输出：用本地编码（GBK），适配 Qt Creator
    if (m_consoleOutput) {
        QByteArray localData = logLine.toLocal8Bit();
        printf("%s\n", localData.constData());
        fflush(stdout);
    }

    // 文件输出：保持 UTF-8
    if (m_fileOutput && m_logFile.isOpen()) {
        QMutexLocker locker(&m_mutex);
        QTextStream stream(&m_logFile);
        stream.setEncoding(QStringConverter::Utf8);
        stream << logLine << "\n";
        stream.flush();
        m_logFile.flush();
    }
}

Logger::Logger(QObject *parent) : QObject(parent)
{

}

Logger::~Logger()
{
    qInstallMessageHandler(nullptr);
    if(m_logFile.isOpen())
    {
        m_logFile.close();
    }
    s_instance = nullptr;
}

void Logger::rotatelog()
{
    QMutexLocker locker(&m_mutex);

    if(m_logFile.isOpen())
    {
        m_logFile.close();
    }

    m_currentFile  = m_logDir + "/app.log";
    m_logFile.setFileName(m_currentFile);
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

}

void Logger::flush()
{
    QMutexLocker locker(&m_mutex);
    if (m_logFile.isOpen())
    {
        m_logFile.flush();
    }
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (!s_instance) return;

    QString level;
    LogLevel lvl = Debug;

    switch (type) {
        case QtDebugMsg:    level = "DEBUG"; lvl = Debug; break;
        case QtInfoMsg:     level = "INFO"; lvl = Info; break;
        case QtWarningMsg:  level = "WARN"; lvl = Warning; break;
        case QtFatalMsg:    level = "FATAL"; lvl = Error; break;
    }

    if (lvl < s_instance->m_minLevel) return;

    // 使用 context.file 和 context.line
    QString file = context.file ? QString(context.file) : "unknown";
    int line = context.line;

    s_instance->writeLog(level, file, line, msg);

    if (type == QtFatalMsg) {
        s_instance->flush();
        abort();
    }
}

void Logger::cleanOldLogs()
{
    QDir baseDir(m_logDir);
    baseDir.cdUp();  // 回到 logs/ 根目录

    QDateTime deadline = QDateTime::currentDateTime().addDays(-7);

    QDirIterator it(baseDir.path(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        QString dirPath = it.next();
        QString dirName = QFileInfo(dirPath).fileName();
        QDateTime dirDate = QDateTime::fromString(dirName, "yyyyMMdd");
        if (dirDate.isValid() && dirDate < deadline) {
            QDir(dirPath).removeRecursively();
        }
    }
}
