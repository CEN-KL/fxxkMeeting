#include "logqueue.h"
#include <QDebug>

LogQueue::LogQueue(QObject *parent) : QThread(parent) { }

void LogQueue::stopImmediately()
{
    QMutexLocker lock(&m_lock);
    m_isCanRun = false;
}

void LogQueue::pushLog(Log *log)
{
    log_queue.push_msg(log);
}

void LogQueue::run()
{
    m_isCanRun = true;
    while (1)
    {
        {
            QMutexLocker lock(&m_lock);
            if (m_isCanRun == false)
            {
                fclose(logfile);
                return;
            }
        }
        Log *log = log_queue.pop_msg();
        if (log == nullptr || log->ptr == nullptr) continue;

        // ---------- write to log file ----------
        errno_t r = fopen_s(&logfile, "./log.txt", "a");
        if (r != 0)
        {
            qDebug() << "打开日志文件失败: " << r;
            continue;
        }

        qint64 hastowrite = log->len;
        qint64 ret = 0, haswrite = 0;
        while ((ret = fwrite(static_cast<char *>(log->ptr) + haswrite, 1, hastowrite - haswrite, logfile)) < hastowrite)
        {
            if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                ret = 0;
            }
            else
            {
                qDebug() << "write logfile error";
                break;
            }
            haswrite += ret;
            hastowrite -= ret;
        }

        if (log->ptr)
            free(log->ptr);
        if (log)
            free(log);

        fflush(logfile);
        fclose(logfile);
    }
}


