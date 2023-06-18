#ifndef SENDIMG_H
#define SENDIMG_H

#include <QThread>
#include <QMutex>
#include <QImage>
#include <QWaitCondition>
#include <QVideoFrame>
#include <QQueue>

class SendImg : public QThread
{
    Q_OBJECT
public:
    SendImg(QObject *par = nullptr);
    void pushToQueue(QImage);
private:
    QQueue<QByteArray> img_queue;
    QMutex queue_lock;
    QMutex m_lock;
    QWaitCondition queue_waitCond;
    volatile bool m_isCanRun;
    void run() override;
public slots:
    void ImageCaptured(QImage);
    void clearImgQueue();
    void stopImmediately();
};

#endif // SENDIMG_H
