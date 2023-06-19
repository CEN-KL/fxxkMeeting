#include "widget.h"
#include "ui_widget.h"

#include <QDebug>
#include <QPainter>
#include <QMessageBox>
#include <QScrollBar>
#include <QHostAddress>
#include <QRegularExpression>
#include <QDateTime>
#include <QCompleter>
#include <QStringListModel>
QRect Widget::pos = QRect(-1, -1, -1, -1);

extern LogQueue *logqueue;

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    // 日志线程
    logqueue = new LogQueue();
    logqueue->start();

    qDebug() << "main: " << QThread::currentThread();
    qRegisterMetaType<MSG_TYPE>();

    WRITE_LOG("------------------------------ Application Start ------------------------------")
    WRITE_LOG("main UI thread in : 0x%p", QThread::currentThreadId());
    // ui界面
    _createmeet = false;
    _joinmeet   = false;
    _openCamera = false;

    Widget::pos = QRect(0.1 * Screen::width, 0.1 * Screen::height, 0.8 * Screen::width, 0.8 * Screen::height);

    ui->setupUi(this);

//    ui->btnAudio->setText(QString(OPENAUDIO).toUtf8());
//    ui->btnVideo->setText(QString(OPENAUDIO).toUtf8());

    this->setGeometry(Widget::pos);
    this->setMinimumSize(QSize(Widget::pos.width() * 0.7, Widget::pos.height() * 0.7));
    this->setMaximumSize(QSize(Widget::pos.width(), Widget::pos.height()));

    // 设置部分按钮为不可用
    ui->btnExit->setDisabled(true);
    ui->btnJoinMeeting->setDisabled(true);
    ui->btnCreate->setDisabled(true);
    ui->btnSend->setDisabled(true);
    ui->btnAudio->setDisabled(true);
    ui->btnVideo->setDisabled(true);

    mainIp = 0; // 主屏幕显示用户ip

    // 直接收发数据线程
    _mytcpsocket = new MyTcpSocket();
    connect(_mytcpsocket, SIGNAL(sendTextOver()), this, SLOT(textSend()));

    // 数据接收并处理
    _recvThread = new RecvSolve();
    connect(_recvThread, SIGNAL(datarecv(MESG*)), this, SLOT(dataSolve(MESG*)));
    _recvThread->start();

    // 传输视频线程
    _sendImg   = new SendImg();
    _imgThread = new QThread();
    _sendImg->moveToThread(_imgThread);
    _sendImg->start();
//    _imgThread->start();  在 on_openVideo_clicked中才被启动
    _frames = new Frames();
    connect(_frames, SIGNAL(imageCaptured(QImage)), this, SLOT(cameraImgCaptured(QImage)));
    connect(this, SIGNAL(pushImg(QImage)), _sendImg, SLOT(ImageCaptured(QImage)));
    connect(_imgThread, SIGNAL(finished()), this, _sendImg, SLOT(clearImgQueue()));


    // 文本传输 包括创建、加入、退出、关闭摄像头信号
    _sendText   = new SendText();
    _textThread = new QThread();
    _sendText->moveToThread(_textThread);
    _sendText->start();
    _textThread->start();
    connect(this, SIGNAL(textReady(MSG_TYPE, QString)), _sendText, SLOT(pushText(MSG_TYPE,QString)));

    // 音频传输
    _ainput       = new AudioInput();
    _ainputThread = new QThread();
    _ainput->moveToThread(_ainputThread);
//    _ainputThread->start(); //获取音频，发送； 在相应槽函数中再启动
    _aoutput      = new AudioOutput();
    _aoutput->start();
    connect(this,     SIGNAL(startAudio()), _ainput, SLOT(startCollect()));
    connect(this,     SIGNAL(stopAudio()),  _ainput, SLOT(stopCollect()));
    connect(_ainput,  SIGNAL(audioInputError(QString)),  this, SLOT(audioError(QString)));
    connect(_aoutput, SIGNAL(audioOutputError(QString)), this, SLOT(audioError(QString)));
    connect(_aoutput, SIGNAL(speaker(QString)), this, SLOT(speaks(QString)));

    // 滚动条
    ui->scrollArea->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { width:8px; background:rgba(0,0,0,0%); margin:0px,0px,0px,0px; padding-top:9px; padding-bottom:9px; } QScrollBar::handle:vertical { width:8px; background:rgba(0,0,0,25%); border-radius:4px; min-height:20; } QScrollBar::handle:vertical:hover { width:8px; background:rgba(0,0,0,50%); border-radius:4px; min-height:20; } QScrollBar::add-line:vertical { height:9px;width:8px; border-image:url(:/images/a/3.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical { height:9px;width:8px; border-image:url(:/images/a/1.png); subcontrol-position:top; } QScrollBar::add-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/4.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/2.png); subcontrol-position:top; } QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:rgba(0,0,0,10%); border-radius:4px; }");
    ui->listWidget->setStyleSheet("QScrollBar:vertical { width:8px; background:rgba(0,0,0,0%); margin:0px,0px,0px,0px; padding-top:9px; padding-bottom:9px; } QScrollBar::handle:vertical { width:8px; background:rgba(0,0,0,25%); border-radius:4px; min-height:20; } QScrollBar::handle:vertical:hover { width:8px; background:rgba(0,0,0,50%); border-radius:4px; min-height:20; } QScrollBar::add-line:vertical { height:9px;width:8px; border-image:url(:/images/a/3.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical { height:9px;width:8px; border-image:url(:/images/a/1.png); subcontrol-position:top; } QScrollBar::add-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/4.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/2.png); subcontrol-position:top; } QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:rgba(0,0,0,10%); border-radius:4px; }");

    QFont text_font = this->font();
    text_font.setFamily("Fira Code");
    text_font.setPointSize(12);

    ui->listWidget->setFont(text_font);

    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setCurrentIndex(0);
}

void Widget::on_btnConnect_clicked()
{
    QString ip = ui->lineIP->text(), port = ui->linePort->text();
    ui->labOutLog->setText("正在连接到" + ip + ":" + port);
    repaint();

    // 检查ip 端口号合法性
    QRegularExpression ipReg("((2{2}[0-3]|2[01][0-9]|1[0-9]{2}|0?[1-9][0-9]|0{0,2}[1-9])\\.)((25[0-5]|2[0-4][0-9]|[01]?[0-9]{0,2})\\.){2}(25[0-5]|2[0-4][0-9]|[01]?[0-9]{1,2})");
    QRegularExpression portReg("^([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])$");
    if (!ipReg.match(ip).isValid())
    {
        QMessageBox::warning(this, "Input Error", "IP error", QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if (!portReg.match(port).isValid())
    {
        QMessageBox::warning(this, "Input Error", "Port error", QMessageBox::Yes, QMessageBox::Yes);
        return;
    }

    if (_mytcpsocket->connectToServer(ip, port, QIODevice::ReadWrite))
    {
        ui->labOutLog->setText("成功连接到" + ip + ":" + port);
        ui->btnAudio->setDisabled(true);
        ui->btnVideo->setDisabled(true);
        ui->btnConnect->setDisabled(true);
        ui->btnExit->setDisabled(true);
        ui->btnCreate->setDisabled(false);
        ui->btnJoinMeeting->setDisabled(false);
        ui->btnSend->setDisabled(false);
        WRITE_LOG("connect to %s:%s successfully", ip.toStdString().c_str(), port.toStdString().c_str());
        QMessageBox::warning(this, "Connection Success", "成功连接服务器", QMessageBox::Yes, QMessageBox::Yes);
    }
    else
    {
        ui->labOutLog->setText("连接失败，请重新连接");
        WRITE_LOG("failed to connenct %s:%s", ip.toStdString().c_str(), port.toStdString().c_str());
        QMessageBox::warning(this, "Connection error", _mytcpSocket->errorString() , QMessageBox::Yes, QMessageBox::Yes);
    }
}

void Widget::on_btnCreate_clicked()
{
    if (!_createmeet)
    {
        ui->btnCreate->setDisabled(true);
        ui->btnAudio->setDisabled(true);
        ui->btnVideo->setDisabled(true);
        ui->btnExit->setDisabled(true);
        emit textReady(CREATE_MEETING);
        WRITE_LOG("create meeting");
    }
}

void Widget::on_btnJoinMeeting_clicked()
{
    QString roomNo = ui->lineJoinMeeting->text();
    QRegularExpression roomReg = ("^[1-9][0-9]{1, 8}$");
    if (!roomReg.match(roomNo).isValid())
        QMessageBox::warning(this, "RoomNo Error", "房间号不合法", QMessageBox::Yes, QMessageBox::Yes);
    else
        emit textReady(JOIN_MEETING, roomNo);
}

void Widget::on_btnExit_clicked()
{
    if (_frames != nullptr)
        _frames->stopCam();
    _createmeet = false;
    _joinmeet = false;
    ui->btnCreate->setDisabled(true);
    ui->btnExit->setDisabled(true);

    clearPartner();

    _mytcpsocket->disconnectFromHost();
    _mytcpsocket->wait();

    ui->labOutLog->setText("已退出会议");
    ui->btnConnect->setDisabled(false);
    ui->groupBoxMainWindow->setTitle("主屏幕");

    while (ui->listWidget->count() > 0)
    {
        QListWidgetItem *item = ui->listWidget->takeItem(0);
        ChatMessage *chat = (ChatMessage*) ui->listWidget->itemWidget(item);
        delete item;
        delete chat;
    }
    _iplist.clear();
    ui->plainTextEdit->setCompleter(_iplist); // 更新自动补全的IP列表

    WRITE_LOG("exit meeting");
    QMessageBox::warning(this, "Informarion", "已退出会议", QMessageBox::Yes, QMessageBox::Yes);
}

void Widget::on_btnAudio_clicked()
{
    if (!_createmeet && !_joinmeet) return;
    if (ui->btnAudio->text().toUtf8() == QString(OPENAUDIO).toUtf8())
    {
        _ainputThread->start();
        emit startAudio();
        ui->btnAudio->setText(QString(CLOSEAUDIO).toUtf8());
    }
    else
    {
        emit stopAudio();
        ui->btnAudio->setText(QString(OPENAUDIO).toUtf8());
    }
}

void Widget::on_btnVideo_clicked()
{
    if (_frames->isCameraActive())
    {
        _frames->stopCam();
        WRITE_LOG("stop camera");
        if (_frames->cameraError() == QCamera::NoError)
        {
            _imgThread->quit();
            _imgThread->wait();
            ui->btnVideo->setText(QString(OPENVIDEO).toUtf8());
            emit textReady(CLOSE_CAMERA);
        }
        closeImg(_mytcpsocket->getLocalIP());
    }
    else
    {
        _frames->initCam();
        WRITE_LOG("open camera");
        if (_frames->cameraError() == QCamera::NoError)
        {
            _imgThread->start();
            ui->btnVideo->setText(QString(CLOSEVIDEO).toUtf8());
        }
    }
}

void Widget::on_btnSend_clicked()
{
    QString msg = ui->plainTextEdit->toPlainTextEdit().trimmed();
    if (msg.size() == 0)
    {
        qDebug() << "empty";
        return;
    }
    qDebug() << msg;
    ui->plainTextEdit->setPlainTextEdit("");
    QString time = QString::number(QDateTime::currentDateTimeUtc().toSecsSinceEpoch());
    ChatMessage *message = new ChatMessageui(ui->listWidget);
    QListWidgetItem *item = new QListWidgetItem();
    dealMessageTime(time);
    dealMessage(message, item, msg, time, QHostAddress(_mytcpsocket->getLocalIP()).toString(), ChatMessage::User_me);
    emit textReady(TEXT_SEND, msg);
    ui->btnSend->setDisabled(true);
}

void Widget::dealMessage(ChatMessage *messageW, QListWidgetItem *item, QString text, QString time, QString ip, ChatMessage::user_type utype)
{
    ui->listWidget->addItem(item);
    messageW->setFixedWidth(ui->listWidget->width());
    QSize size = messageW->fontRect(text);
    item->setSizeHint(size);
    messageW->setText(text, time, size, ip, utype);
    ui->listWidget->setItemWidget(item, messageW);
}

void Widget::dealMessageTime(QString curMsgTime)
{
    bool isShowTime = false;
    if(ui->listWidget->count() > 0)
    {
        QListWidgetItem* lastItem = ui->listWidget->item(ui->listWidget->count() - 1);
        ChatMessage* messageW = (ChatMessage *)ui->listWidget->itemWidget(lastItem);
        int lastTime = messageW->time().toInt();
        int curTime = curMsgTime.toInt();
        qDebug() << "curTime lastTime:" << curTime - lastTime;
        isShowTime = ((curTime - lastTime) > 60); // 两个消息相差一分钟
        //        isShowTime = true;
    } else
    {
        isShowTime = true;
    }

    if(isShowTime) {
        ChatMessage* messageTime = new ChatMessage(ui->listWidget);
        QListWidgetItem* itemTime = new QListWidgetItem();
        ui->listWidget->addItem(itemTime);
        QSize size = QSize(ui->listWidget->width() , 40);
        messageTime->resize(size);
        itemTime->setSizeHint(size);
        messageTime->setText(curMsgTime, curMsgTime, size);
        ui->listWidget->setItemWidget(itemTime, messageTime);
    }
}

void Widget::cameraImgCaptured(QImage img)
{
    QTransform matrix;
    matrix.rotate(180.0);
    QImage image = img.transformed(matrix, Qt::FastTransformation).scaled(ui->labMainWindow->size());
    if (_partner.size() > 1)
        emit pushImg(image);
    if (_mytcpsocket->getLocalIP() == mainIp)
        ui->labMainWindow->setPixmap(QPixmap::fromImage(image).scaled(ui->labMainWindow->size()));

    // 副屏幕画面
    Partner * p = _partner[_mytcpsocket->getLocalIP()];
    if (p)
        p->setPic(image);
}

Widget::~Widget()
{
    //终止底层发送与接收线程

    if(_mytcpSocket->isRunning())
    {
        _mytcpSocket->stopImmediately();
        _mytcpSocket->wait();
    }

    //终止接收处理线程
    if(_recvThread->isRunning())
    {
        _recvThread->stopImmediately();
        _recvThread->wait();
    }

    if(_imgThread->isRunning())
    {
        _imgThread->quit();
        _imgThread->wait();
    }

    if(_sendImg->isRunning())
    {
        _sendImg->stopImmediately();
        _sendImg->wait();
    }

    if(_textThread->isRunning())
    {
        _textThread->quit();
        _textThread->wait();
    }

    if(_sendText->isRunning())
    {
        _sendText->stopImmediately();
        _sendText->wait();
    }

    if (_ainputThread->isRunning())
    {
        _ainputThread->quit();
        _ainputThread->wait();
    }

    if (_aoutput->isRunning())
    {
        _aoutput->stopImmediately();
        _aoutput->wait();
    }
    WRITE_LOG("-------------------Application End-----------------");

    //关闭日志
    if(logqueue->isRunning())
    {
        logqueue->stopImmediately();
        logqueue->wait();
    }

    delete ui;
}

