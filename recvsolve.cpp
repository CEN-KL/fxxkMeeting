#include "recvsolve.h"
#include <QDebug>
extern QUEUE_DATA<MESG> queue_recv;

RecvSolve::RecvSolve(QObject *par) : QThread(par)
{
    qRegisterMetaType<MESG *>();
    m_isCanRun = true;
}

void RecvSolve::run()
{
    WRITE_LOG("start solving data thread: 0x%p", QThread::currentThreadId());
    while (true)
    {
        {
            QMutexLocker locker(&m_lock);
            if (m_isCanRun == false)
            {
                WRITE_LOG("stop solving data thread: 0x%p", QThread::currentThreadId());
                return;
            }
        }
        MESG *msg = queue_recv.pop_msg();
        if (msg == nullptr) continue;
        qDebug() << "recvSolve msg_type: " + msg->msg_type;
        emit datarecv(msg);
    }
}

void RecvSolve::stopImmediately()
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;
}
