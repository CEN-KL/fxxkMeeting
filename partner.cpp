#include "partner.h"
#include <QDebug>
#include <QHostAddress>

Partner::Partner(QWidget *par, quint32 p): QLabel(par), ip(p)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    w = ((QWidget *)this->parent())->size().width();
    this->setPixmap(QPixmap::fromImage(QImage(":/myImage/1.jpg").scaled(w - 10, w - 10)));
    this->setFrameShape(QFrame::Box);

    this->setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(0, 0, 255, 0.7)");
    this->setToolTip(QHostAddress(ip).toString()); // 悬浮窗提示ip
}

void Partner::mousePressEvent(QMouseEvent *)
{
    emit sendIp(ip);
}

void Partner::setPic(QImage img)
{
    this->setPixmap(QPixmap::fromImage(img.scaled(w - 10, w - 10)));
}
