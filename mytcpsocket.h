#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QMutex>
#include <QThread>
#include <QTcpSocket>
#include "netheader.h"

typedef unsigned char uchar;

/*
 * 负责底层数据的直接接收和发送
 */
class MyTcpSocket : public QThread
{
    Q_OBJECT
public:
    ~MyTcpSocket();
    MyTcpSocket(QObject *par = nullptr);
    bool connectToServer(QString, QString, QIODevice::OpenModeFlag);
    QString errorString();
    void disconnectFromHost();
    quint32 getLocalIP();
private:
    void run() override;
    qint64 readn(char *, quint64, int);
    QTcpSocket *_socktcp;
    QThread *_sockThread;
    uchar *sendbuf;
    uchar *recvbuf;
    quint64 hasrecv;

    QMutex m_lock;
    volatile bool m_isCanRun;
private slots:
    bool connectServer(QString, QString, QIODevice::OpenModeFlag);
    void sendData(MESG *);
    void closeSocket();

public slots:
    void recvFromSocket();
    void stopImmedeiately();
    void errorDetect(QAbstractSocket::SocketError);
signals:
    void socketError(QAbstractSocket::SocketError);
    void sendTextOver();
};

inline QString MyTcpSocket::errorString() { return _socktcp->errorString(); }

inline quint32 MyTcpSocket::getLocalIP()
{
    if (_socktcp->isOpen())
    {
        return _socktcp->localAddress().toIPv4Address();
    }
    else{
        return -1;
    }
}

#endif // MYTCPSOCKET_H
