#include "mytcpsocket.h"

MyTcpSocket::~MyTcpSocket()
{
    free(sendbuf);
    free(recvbuf);
    delete _sockThread;
}

MyTcpSocket::MyTcpSocket(QObject *par): QThread(par)
{
    qRegisterMetaType<QAbstractSocket::SocketError>();
    _socket = nullptr;
    _sockThread = new QThread();
    this->moveToThread(_sockThread); //负责数据发送
    connect(_sockThread, SIGNAL(finished()), this, SLOT(closeSocket()));
    sendbuf = (uchar *) malloc(4 * MB);
    recvbuf = (uchar *) malloc(4 * MB);
    hasrecv = 0;
}

bool MyTcpSocket::connectToServer(QString, QString, QIODeviceBase::OpenModeFlag)
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

bool MyTcpSocket::connectServer(QString ip, QString port, QIODeviceBase::OpenModeFlag)
{
    if (_socktcp == nullptr) _socktcp = new QTcpSocket();
    // 以异步的方式连接到指定ip和端口的服务器，成功后会发送connected信号
    _socktcp->connectToHost(ip, port.toUShort(), flag);
    // readReady -- 当缓冲区有新数据需要读取时此信号被发射
    connect(_socktcp, SIGNAL(readReady()), this, SLOT(recvFromSocket()), Qt::UniqueConnection);
    // 处理socket错误
    connect(_socktcp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorDetect(QAbstractSocket::SocketError)), Qt::UniqueConnection);

    if (_socktcp->waitForConnected(5000))
        return true;
    _socktcp->close;
    return false;
}


