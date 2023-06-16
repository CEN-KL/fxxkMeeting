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

}
