#ifndef RECVSOLVE_H
#define RECVSOLVE_H
#include <QThread>
#include <QMutex>
#include "netheader.h"

class RecvSolve : public QThread
{
    Q_OBJECT
public:
    RecvSolve(QObject *par = nullptr);
    void run() override;
private:
    QMutex m_lock;
    bool m_isCanRun;
signals:
    void datarecv(MESG *);
public slots:
    void stopImmediately();
};

#endif // RECVSOLVE_H
