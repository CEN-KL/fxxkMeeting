#ifndef LOGQUEUE_H
#define LOGQUEUE_H

#include <QThread>
#include <QMutex>
#include <queue>
#include "netheader.h"


class LogQueue : public QThread
{
public:
    explicit LogQueue(QObject *parent = nullptr);
    void stopImmediately();
    void pushLog(Log *);
private:
    void run() override;
    QMutex m_lock;
    bool m_isCanRun;
    QUEUE_DATA<Log> log_queue;
    FILE *logfile;
};

#endif // LOGQUEUE_H
