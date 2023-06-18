#ifndef NETHEADER_H
#define NETHEADER_H

#include <QMetaType>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include <QWaitCondition>

const quint32 QUEUE_MAXSIZE = 1500;
const quint32 WAITSECONDS = 2;
constexpr quint32 MB = 1024 * 1024;
constexpr quint32 KB = 1024;

#ifndef WAITSECONDS
#define WAITSECONDS 2
#endif

#ifndef OPENVIDEO
#define OPENVIDEO "打开视频"
#endif

#ifndef CLOSEVIDEO
#define CLOSEVIDEO "关闭视频"
#endif

#ifndef OPENAUDIO
#define OPENAUDIO "打开音频"
#endif

#ifndef CLOSEAUDIO
#define CLOSEAUDIO "关闭音频"
#endif

#ifndef MSG_HEADER
#define MSG_HEADER 11
#endif

enum MSG_TYPE
{
    IMG_SEND = 0,
    IMG_RECV,
    AUDIO_SEND,
    AUDIO_RECV,
    TEXT_SEND,
    TEXT_RECV,
    CREATE_MEETING,
    EXIT_MEETING,
    JOIN_MEETING,
    CLOSE_CAMERA,

    CREATE_MEETING_RESPONSE = 20,
    PARTNER_EXIT = 21,
    PARTNER_JOIN = 22,
    JOIN_MEETING_RESPONSE = 23,
    PARTNER_JOIN2 = 24,
    RemoteHostClosedError = 40,
    OtherNetError = 41
};
Q_DECLARE_METATYPE(MSG_TYPE);

struct MESG  // 消息结构体
{
    MSG_TYPE msg_type;
    uchar *data;
    quint32 len;
    quint32 ip;
};
Q_DECLARE_METATYPE(MESG *);

// -----------------------------

template<typename T>
struct QUEUE_DATA
{
private:
    QMutex send_queueLock;
    QWaitCondition send_queueCond;
    QQueue<T*> send_queue;
public:
    void push_msg(T *msg)
    {
        send_queueLock.lock();
        while (send_queue.size() > QUEUE_MAXSIZE)
        {
            send_queueCond.wait(&send_queueLock); // 释放互斥锁，等待唤醒
        }
        send_queue.push_back(msg);
        send_queueLock.unlock();
        send_queueCond.wakeOne();
    }

    T* pop_msg()
    {
        send_queueLock.lock();
        while (send_queue.size() == 0)
        {
            bool f = send_queueCond.wait(&send_queueLock, WAITSECONDS * 1000);
            if (f == false)
            {
                send_queueLock.unlock();
                return nullptr;
            }
        }
        T *send = send_queue.front();
        send_queue.pop_front();
        send_queueLock.unlock();
        send_queueCond.wakeOne();
        return send;
    }

    void clear()
    {
        send_queueLock.lock();
        send_queue.clear();
        send_queueLock.unlock();
    }
};

struct Log
{
    char *ptr;
    qint64 len;
};

void log_print(const char *, const char *, int, const char *, ...);
#define WRITE_LOG(LOGTEXT, ...) do{ \
log_print(__FILE__, __FUNCTION__, __LINE__, LOGTEXT, ##__VA_ARGS__);\
}while(0);

#endif // NETHEADER_H
