#include "sendtext.h"
#include <QDebug>

extern QUEUE_DATA<MESG> queue_send;

SendText::SendText(QObject *par) : QThread(par)
{

}

void SendText::stopImmediately()
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;
}

void SendText::pushText(MSG_TYPE msgType, QString str)
{
    queue_lock.lock();

    while (textqueue.size() > QUEUE_MAXSIZE)
    {
        queue_waitCond.wait(&queue_lock);
    }
    textqueue.push_back(M(str, msgType));

    queue_lock.unlock();
    queue_waitCond.wakeOne();
}

void SendText::run()
{
    m_isCanRun = true;
    WRITE_LOG("start sending text thread: 0x%p", QThread::currentThreadId());
    while (true)
    {
        queue_lock.lock();
        while (textqueue.size() == 0)
        {
            bool f = queue_waitCond.wait(&queue_lock, WAITSECONDS * 1000);
            if (!f)
            {
                QMutexLocker locker(&m_lock);
                if (!m_isCanRun)
                {
                    queue_lock.unlock();
                    WRITE_LOG("stop sending text thread: 0x%p", QThread::currentThreadId());
                    return;
                }
            }
        }

        M text = textqueue.front();
        textqueue.pop_front();
        queue_lock.unlock();
        queue_waitCond.wakeOne();

        // 构造消息
        MESG * send = new MESG();
        memset(send, 0, sizeof(MESG));
        send->msg_type = text.type;
        if (text.type == CREATE_MEETING || text.type == CLOSE_CAMERA)
        {
            send->len = 0;
            send->data = nullptr;
        }
        else if (text.type == JOIN_MEETING)
        {
            send->len = sizeof(quint32);
            send->data = new uchar[send->len + 10];
            memset(send->data, 0, send->len + 10);
            quint32 roomNo = text.str.toUInt();
            memcpy(send->data, &roomNo, sizeof(roomNo));
        }
        else if (text.type == TEXT_SEND)
        {
            QByteArray data = qCompress(QByteArray::fromStdString(text.str.toStdString()));
            send->len = data.size();
            send->data = new uchar[send->len];
            memset(send->data, 0, send->len);
            memcpy(send->data, data.data(), data.size());
        }
        queue_send.push_msg(send);  // send的内存位置会在MyTcpSocket::sendData中释放
    }
}
