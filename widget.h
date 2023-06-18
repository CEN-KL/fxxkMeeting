#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QCamera>
#include <QImageCapture>
#include <QMap>

#include "sendimg.h"
#include "recvsolve.h"
#include "sendtext.h"
#include "mytcpsocket.h"
#include "audioinput.h"
#include "audiooutput.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;

    static QRect pos;
    quint32 mainIp; //主屏幕显示的IP图像
    QCamera *_camera;
    QImageCapture *_imgcapture; //截屏
    bool _createmeet; // 是否创建会议
    bool _joinmeet;   // 是否加入会议
    bool _openCamera; // 是否打开摄像头

    // MyVideoSurface *_myvideosurface;

    QVideoFrame mainshow;

    SendImg *_sendImg;
    QThread *_imgThread;

    RecvSolve *_recvThread;

    SendText *_sendText;
    QThread *_textThread;

    MyTcpSocket *_mytcpsocket;
//    void paintEvent(QPaintEvent *event);

    //QMap

    // deal message

    AudioInput *_ainput;
    QThread *_ainputThread;
    AudioOutput *aoutput;

    QStringList iplist;

};
#endif // WIDGET_H
