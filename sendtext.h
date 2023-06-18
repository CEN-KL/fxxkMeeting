#ifndef SENDTEXT_H
#define SENDTEXT_H

#include "netheader.h"
#include <QThread>
#include <QMutex>
#include <QQueue>

struct M
{
    QString str;
    MSG_TYPE type;

    M(QString s, MSG_TYPE e) : str(s), type(e) { }
};

class SendText : public QThread
{
    Q_OBJECT
public:
    SendText(QObject *par = nullptr);
    ~SendText() = default;
private:
    QQueue<M> textqueue;
    QMutex m_lock;
    QMutex queue_lock;
    QWaitCondition queue_waitCond;
    volatile bool m_isCanRun;

    void run() override;
public slots:
    void pushText(MSG_TYPE, QString = "");
    void stopImmediately();
};

#endif // SENDTEXT_H
