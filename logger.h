#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QMutex>
#include <QTimer>


class Logger : public QObject
{
    Q_OBJECT
public:

    enum LogLevel{Debug, Info, Warning, Error};
        Q_ENUM(LogLevel)

        static Logger* instance();

        // 初始化日志系统
        void init(const QString dir = "logs", bool consoleOutput = true,  bool fileOutput = true);

        // 设置日志级别过滤
        void setMinLevel(LogLevel level);

        // 获取日志路径
        QString currentLogFile() const;

signals:


public slots:



private:
        explicit Logger(QObject *parent = nullptr);
            ~Logger();

            // 写入文件
            void writeLog(const QString &level, const QString &file, int line, const QString &msg);
            // 创建实时日志
            void rotatelog();
            // 销毁日志
            void flush();
            // 日志助手
            static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

            void cleanOldLogs();

            QString m_logDir;
            QString m_currentFile;
            QFile m_logFile;
            QMutex m_mutex;
            LogLevel m_minLevel = Debug;
            bool m_consoleOutput = true;
            bool m_fileOutput = true;
            qint64 m_maxFileSize = 50 * 1024 * 1024;
            static Logger *s_instance;

};

#endif // LOGGER_H
