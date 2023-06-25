#include "mytcpsocket.h"
#include <QtEndian>
#include <string>

extern QUEUE_DATA<MESG> queue_send;
extern QUEUE_DATA<MESG> queue_recv;
extern QUEUE_DATA<MESG> audio_recv;

MyTcpSocket::~MyTcpSocket()
{
    free(sendbuf);
    free(recvbuf);
    delete _sockThread;
}

MyTcpSocket::MyTcpSocket(QObject *par): QThread(par)
{
    qRegisterMetaType<QAbstractSocket::SocketError>();
    _socktcp = nullptr;
    _sockThread = new QThread();
    this->moveToThread(_sockThread); //负责数据发送
    connect(_sockThread, SIGNAL(finished()), this, SLOT(closeSocket()));
    sendbuf = (uchar *) malloc(4 * MB);
    recvbuf = (uchar *) malloc(4 * MB);
    hasrecv = 0;
}

bool MyTcpSocket::connectToServer(QString ip, QString port, QIODeviceBase::OpenModeFlag flag)
{
    _sockThread->start();
    bool retVal;
    QMetaObject::invokeMethod(this, "connectServer", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, retVal),
                              Q_ARG(QString, ip), Q_ARG(QString, port), Q_ARG(QIODevice::OpenModeFlag, flag));

    if (retVal)
    {
        this->start();  // run方法负责调用sendData发送数据
        return true;
    }
    return false;
}

bool MyTcpSocket::connectServer(QString ip, QString port, QIODeviceBase::OpenModeFlag flag)
{
    if (_socktcp == nullptr) _socktcp = new QTcpSocket();
    // readReady -- 当缓冲区有新数据需要读取时此信号被发射
    connect(_socktcp, SIGNAL(readyRead()), this, SLOT(recvFromSocket()), Qt::UniqueConnection);
    // 处理socket错误
//    connect(_socktcp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorDetect(QAbstractSocket::SocketError)), Qt::UniqueConnection);
    connect(_socktcp, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(errorDetect(QAbstractSocket::SocketError)), Qt::UniqueConnection);

    // 以异步的方式连接到指定ip和端口的服务器，成功后会发送connected信号
    _socktcp->connectToHost(ip, port.toUShort(), flag);
    bool isConnected = _socktcp->waitForConnected(5000);
    qDebug() << "isConnected: " << isConnected;
    if (!isConnected) {
        qDebug() << "connectServer returns false";
        _socktcp->close();
    }
    return isConnected;
}

void MyTcpSocket::closeSocket()
{
    if (_socktcp && _socktcp->isOpen())
        _socktcp->close();
}

void MyTcpSocket::recvFromSocket()
{
    /*
     * 接收服务端的数据并转化成客户端的MESG格式，送入recv_queue
    */
    qint64 availbytes = _socktcp->bytesAvailable();
    if (availbytes <= 0)
        return;

    qint64 ret = _socktcp->read((char *) recvbuf + hasrecv, availbytes);
    if (ret <= 0)
    {
        qDebug() << "error or no more data";
        return;
    }
    hasrecv += ret;

    if (hasrecv < MSG_HEADER)
    {
        return; // 收到的数据包大小不足一个报头
    }
    else
    {
        quint32 data_len;
        qFromBigEndian<quint32>(recvbuf + 7, 4, &data_len);
        quint32 data_size = data_len;
        if (hasrecv < static_cast<quint64>(data_size) + 1 + MSG_HEADER)
            return;
        if (recvbuf[0] == '$' && recvbuf[MSG_HEADER + data_size] == '#')
        {
            uint16_t type;
            qFromBigEndian<uint16_t>(recvbuf + 1, 2, &type);
            MSG_TYPE msgtype = static_cast<MSG_TYPE>(type);
            qDebug() << "recv data type: " << msgtype;
            // 分别处理不同类型的消息
            if (msgtype == CREATE_MEETING_RESPONSE)
            {
                qint32 roomNo;
                qFromBigEndian<qint32>(recvbuf + MSG_HEADER, 4, &roomNo);

                MESG *msg = new MESG();
                if (msg == nullptr)
                {
                    qDebug() << __LINE__ << "CREATING_MEETING_RESPONSE new MESG failed";
                }
                else
                {
                    memset(msg, 0, sizeof(MESG));
                    msg->msg_type = msgtype;
                    msg->data = new uchar[data_size];
                    if (msg->data == nullptr)
                    {
                        delete msg;
                        qDebug() << __LINE__ << "CREATING_MEETING_RESPONSE new MESG.data failed";
                    }
                    else
                    {
                        memset(msg->data, 0, data_size);
                        memcpy(msg->data, &roomNo, sizeof(roomNo));
                        msg->len = data_size;
                        queue_recv.push_msg(msg);
                    }
                }
            }
            else if (msgtype == JOIN_MEETING_RESPONSE)
            {
                qint32 c;
                memcpy(&c, recvbuf + MSG_HEADER, data_size);

                MESG* msg = new MESG();
                if (msg == nullptr)
                {
                    qDebug() << __LINE__ << "JOIN_MEETING_RESPONSE new MESG failed";
                }
                else
                {
                    memset(msg, 0, sizeof(MESG));
                    msg->msg_type = msgtype;
                    msg->data = new uchar[data_size];
                    if (msg->data == nullptr)
                    {
                        delete msg;
                        qDebug() << __LINE__ << "JOIN_MEETING_RESPONSE new MESG.data failed";
                    }
                    else
                    {
                        memset(msg->data, 0, data_size);
                        memcpy(msg->data, &c, data_size);

                        msg->len = data_size;
                        queue_recv.push_msg(msg);
                    }
                }
            }
            else if (msgtype == PARTNER_JOIN2)
            {
                MESG *msg = new MESG();
                if (msg == nullptr)
                {
                    qDebug() << "PARTNER_JOIN2 new MESG error";
                }
                else
                {
                    memset(msg, 0, sizeof(MESG));
                    msg->msg_type = msgtype;
                    msg->len = data_size;
                    msg->data = new uchar[data_size];
                    if (msg->data == nullptr)
                    {
                        delete msg;
                        qDebug() << "PARTNER_JOIN2 new MESG.data error";
                    }
                    else
                    {
                        memset(msg->data, 0, data_size);
                        uint32_t ip;
                        int pos = 0;
                        for (decltype(data_size) i = 0; i < data_size / sizeof(uint32_t); i++)
                        {
                            qFromBigEndian<uint32_t>(recvbuf + MSG_HEADER + pos, sizeof(uint32_t), &ip);
                            memcpy_s(msg->data + pos, data_size - pos, &ip, sizeof(uint32_t));
                            pos += sizeof(uint32_t);
                        }
                        queue_recv.push_msg(msg);
                    }
                }
            }
            else if (msgtype == IMG_RECV || msgtype == AUDIO_RECV || msgtype == TEXT_RECV || msgtype == PARTNER_JOIN || msgtype == PARTNER_EXIT || msgtype == CLOSE_CAMERA)
            {
                quint32 ip;
                qFromBigEndian<quint32>(recvbuf + 3, 4, &ip);

                if (msgtype == IMG_RECV || msgtype == AUDIO_RECV)
                {
                    QByteArray cc((char *)recvbuf + MSG_HEADER, data_size);
                    QByteArray rc = QByteArray::fromBase64(cc);
                    QByteArray rdc = qUncompress(rc);

                    if (rdc.size() > 0)
                    {
                        MESG *msg = new MESG();
                        memset(msg, 0, sizeof(MESG));
                        msg->msg_type = msgtype;
                        msg->data = new uchar[rdc.size()];
                        memset(msg->data, 0, rdc.size());
                        memcpy_s(msg->data, rdc.size(), rdc.data(), rdc.size());
                        msg->len = rdc.size();
                        msg->ip = ip;
                        if (msgtype == IMG_RECV)
                            queue_recv.push_msg(msg);
                        else
                            audio_recv.push_msg(msg);
                    }
                }
                else if (msgtype == TEXT_RECV)
                {
                    QByteArray cc((char *)recvbuf + MSG_HEADER, data_size);
                    std::string rr = qUncompress(cc).toStdString();
                    if (rr.size() > 0)
                    {
                        MESG *msg = new MESG();
                        memset(msg, 0, sizeof(MESG));
                        msg->msg_type = msgtype;
                        msg->ip = ip;
                        msg->data = new uchar[rr.size()];
                        memset(msg->data, 0, rr.size());
                        memcpy_s(msg->data, rr.size(), rr.data(), rr.size());
                        msg->len = rr.size();
                        queue_recv.push_msg(msg);
                    }
                }
                else if (msgtype == PARTNER_JOIN || msgtype == PARTNER_EXIT || msgtype == CLOSE_CAMERA)
                {
                    MESG *msg = new MESG();
                    memset(msg, 0, sizeof(MESG));
                    msg->msg_type = msgtype;
                    msg->ip = ip;
                    queue_recv.push_msg(msg);
                }
            }
            else
            {
                qDebug() << "msg type error";
            }
            hasrecv -= static_cast<quint64>(data_size + 1 + MSG_HEADER);
            memmove_s(recvbuf, 4 * MB, recvbuf + MSG_HEADER + data_size + 1, hasrecv);
        }
        else
        {
            qDebug() << "pakage error";
//            qDebug() << "recvbuf[0] == " << static_cast<uchar>(recvbuf[0]);
//            qDebug() << "data_size == " << data_size;
//            qDebug() << "recvbuf[-1] == " << static_cast<uchar>(recvbuf[MSG_HEADER + data_size]);
        }
    }
}

void MyTcpSocket::run()
{
    /*
     * 处理数据发送
    */
    m_isCanRun = true;
    while (true)
    {
        {
            QMutexLocker locker(&m_lock);
            if (!m_isCanRun) return;
        }
        MESG *send = queue_send.pop_msg();
        if (send == nullptr) continue;
        QMetaObject::invokeMethod(this, "sendData", Q_ARG(MESG*, send));
    }
}

void MyTcpSocket::sendData(MESG *send)
{
    if (_socktcp->state() == QAbstractSocket::UnconnectedState)
    {
        emit sendTextOver();
        if (send->data) delete send->data;
        if (send) delete send;
        return;
    }
    // ------------------ 组装消息 -----------------------
    quint64 bytes_to_write = 0;
    // 第一个字节永远是 $
    sendbuf[bytes_to_write++] = '$';
    //消息类型 2字节
    qToBigEndian<quint16>(send->msg_type, sendbuf + bytes_to_write);
    bytes_to_write += 2;
    // 发送者ip
    quint32 ip = _socktcp->localAddress().toIPv4Address();
    qToBigEndian<quint32>(ip, sendbuf + bytes_to_write);
    bytes_to_write += 4;

    if (send->msg_type == IMG_SEND || send->msg_type == AUDIO_SEND || send->msg_type == TEXT_SEND || send->msg_type == CLOSE_CAMERA || send->msg_type == CREATE_MEETING)
    {
        qToBigEndian<quint32>(send->len, sendbuf + bytes_to_write);
        bytes_to_write += 4;
    }
    else if(send->msg_type == JOIN_MEETING)
    {
        qToBigEndian<quint32>(send->len, sendbuf + bytes_to_write);
        bytes_to_write += 4;
        uint32_t room;
        memcpy(&room, send->data, send->len);
        qToBigEndian<quint32>(room, send->data);
    }

    // 数据拷入sendbuf
    memcpy(sendbuf + bytes_to_write, send->data, send->len);
    bytes_to_write += send->len;
    sendbuf[bytes_to_write++] = '#';

    // ------------------- write to server ----------------------
    qint64 hastowrite = bytes_to_write;
    qint64 ret = 0, haswrite = 0;
    while ((ret = _socktcp->write((char *)sendbuf + haswrite, hastowrite - haswrite)) < hastowrite)
    {
        if (ret == -1)
        {
            if (_socktcp->error() == QAbstractSocket::TemporaryError)
            {
                ret = 0;
            }
            else
            {
                qDebug() << "network error";
                break;
            }
        }
        haswrite += ret;
        hastowrite -= ret;
    }

    _socktcp->waitForBytesWritten(); // This function blocks until at least one byte has been written on the socket and the bytesWritten() signal has been emitted. The function will timeout after msecs milliseconds; the default timeout is 30000 milliseconds.

    if (send->msg_type == TEXT_SEND)
    {
        emit sendTextOver();
    }

    if (send->data)
        delete send->data;
    if (send)
        delete send;
}

void MyTcpSocket::errorDetect(QAbstractSocket::SocketError error)
{
    qDebug() << "sock error" << QThread::currentThreadId();
    MESG *msg = new MESG();
    memset(msg, 0, sizeof(MESG));
    if (error == QAbstractSocket::RemoteHostClosedError)
    {
        qDebug() << "RemoteHostClosedError";
        msg->msg_type = RemoteHostClosedError;
    }
    else
    {
        qDebug() << "OtherNetError";
        msg->msg_type = OtherNetError;
    }
    queue_recv.push_msg(msg);
}

void MyTcpSocket::stopImmedeiately()
{
    {
        QMutexLocker locker(&m_lock);
        m_isCanRun = false;
    }
    _sockThread->quit();
    _sockThread->wait();
}

qint64 MyTcpSocket::readn(char *buf, quint64 maxsize, int n)
{
    quint64 hastoread = n;
    quint64 hasread = 0;
    do
    {
        qint64 ret = _socktcp->read(buf + hasread, hastoread);
        if (ret < 0)
        {
            return -1;
        }
        if (ret == 0)
        {
            return hasread;
        }
        hasread += ret;
        hastoread -= ret;
    } while (hastoread > 0 && hasread < maxsize);
    return hasread;
}

void MyTcpSocket::disconnectFromHost()
{
    if (this->isRunning())  // 这里是关闭写线程，因为是由run方法进行的
    {
        QMutexLocker locker(&m_lock);
        m_isCanRun = false;
    }

    if (_sockThread->isRunning()) // 这里是关闭读线程 在构造函数中被moveToThread了
    {
        _sockThread->quit();
        _sockThread->wait();
    }

    queue_send.clear();
    queue_recv.clear();
    audio_recv.clear();
}
