#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QCamera>
#include <QImageCapture>
#include <QMap>
#include <QStringListModel>
#include <QListWidgetItem>
#include <QPointer>
#include <QSoundEffect>
#include <QVBoxLayout>

#include "sendimg.h"
#include "recvsolve.h"
#include "sendtext.h"
#include "mytcpsocket.h"
#include "audioinput.h"
#include "audiooutput.h"
#include "chatmessage.h"
#include "partner.h"
#include "frames.h"
#include "screen.h"
#include "logqueue.h"


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
//    QImageCapture *_imgcapture; //截屏
    bool _createmeet; // 是否创建会议
    bool _joinmeet;   // 是否加入会议
    bool _openCamera; // 是否打开摄像头

    // 直接与服务器进行数据收发
    MyTcpSocket *_mytcpsocket;

    // 视频帧发送、接收
    Frames * _frames;
    QVideoFrame _mainshow;
    SendImg *_sendImg;
    QThread *_imgThread;

    // 接收来自MyTcpSocket传来的MESG数据
    RecvSolve *_recvThread;

    // 文本数据传输
    SendText *_sendText;
    QThread *_textThread;

    void paintEvent(QPaintEvent *event);

    QMap<quint32, Partner *> _partner; // 记录房间用户
    Partner *addPartner(const quint32);
    void removePartner(const quint32);
    void clearPartner();
    void closeImg(const quint32);
    QPointer<QVBoxLayout> vlayoutScroll;

    // deal message
    void dealMessage(ChatMessage *messageW, QListWidgetItem *item, QString text, QString time, QString ip, ChatMessage::user_type utype);
    void dealMessageTime(QString curMsgTime);

    AudioInput *_ainput;
    QThread *_ainputThread;
    AudioOutput *_aoutput;
    QPointer<QSoundEffect> _soundEffect; //音效

    QStringList _iplist;

private slots:
    void on_btnCreate_clicked();
    void on_btnExit_clicked();
    void on_btnJoinMeeting_clicked();
    void on_btnConnect_clicked();
    void on_btnAudio_clicked();
    void on_btnVideo_clicked();
    void on_btnSend_clicked();
    void on_sliderVol_valueChanged(int val) { emit volumnChange(val); }// 音量条

    void textSend(); // connected with MyTcpSocket::sendTextOver()
    void cameraImgCaptured(QImage); //connectd with Frames::imageCpatured(QImage)
    void dataSolve(MESG *); // connected with RecvSolve::datarecv(MESG *);
    void audioError(QString);
    void speaks(const QString);
    void recvip(quint32);

signals:
    void textReady(MSG_TYPE, QString = "");
    void pushImg(QImage);  // emitted on slot Widget::cameraImgCaptured(QImage)
    void startAudio();
    void stopAudio();
    void volumnChange(int);
};
#endif // WIDGET_H
