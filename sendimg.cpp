#include "sendimg.h"
#include "netheader.h"
#include <QDebug>
#include <QBuffer>

extern QUEUE_DATA<MESG> queue_send;

SendImg::SendImg(QObject *par) : QThread(par)
{

}

// 消费线程
void SendImg::run()
{
    WRITE_LOG("start sending picture thread: 0x%p", QThread::currentThreadId());
    m_isCanRun = true;
    while (true)
    {
        queue_lock.lock();
        while (img_queue.size() == 0)
        {
            bool f = queue_waitCond.wait(&queue_lock, WAITSECONDS * 1000);
            if (f == false)
            {
                QMutexLocker locker(&m_lock);
                if (m_isCanRun == false)
                {
                    queue_lock.unlock();
                    WRITE_LOG("stop sending picture thread: 0x%p", QThread::currentThreadId());
                    return;
                }
            }
        }
        QByteArray img = img_queue.front();
        img_queue.pop_front();
        queue_lock.unlock();
        queue_waitCond.wakeOne();

        MESG *imgsend = new MESG();
        memset(imgsend, 0, sizeof(MESG));
        imgsend->msg_type = IMG_SEND;
        imgsend->len = img.size();
        qDebug() << "img size: " << img.size();
        imgsend->data = new uchar[imgsend->len];
        memset(imgsend->data, 0, imgsend->len);
        memcpy_s(imgsend->data, imgsend->len, img.data(), img.size());
        queue_send.push_msg(imgsend);
    }
}

// 生产线程
void SendImg::pushToQueue(QImage img)
{
    QByteArray byte;
    QBuffer buf(&byte);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    QByteArray ss = qCompress(byte);
    QByteArray vv = ss.toBase64();
    queue_lock.lock();
    while (img_queue.size() > QUEUE_MAXSIZE)
    {
        queue_waitCond.wait(&queue_lock);
    }
    img_queue.push_back(vv);
    queue_lock.unlock();
    queue_waitCond.wakeOne();
}

void SendImg::ImageCaptured(QImage img)
{
    pushToQueue(img);
}

void SendImg::clearImgQueue()
{
    qDebug() << "清空视频队列";
    QMutexLocker locker(&queue_lock);
    img_queue.clear();
}

void SendImg::stopImmediately()
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;
}
