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
#include <QPixmap>
QRect Widget::pos = QRect(-1, -1, -1, -1);

extern LogQueue *logqueue;

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
//    qDebug() << "Wdiget::Widget tid = " << QThread::currentThreadId();
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
    connect(_recvThread, SIGNAL(datarecv(MESG*)), this, SLOT(dataSolve(MESG*)), Qt::BlockingQueuedConnection);
    _recvThread->start();

    // 传输视频线程
    _sendImg   = new SendImg();
    _imgThread = new QThread();
    _sendImg->moveToThread(_imgThread);
    _sendImg->start();
//    _imgThread->start();  在 on_openVideo_clicked中才被启动
    _camera = new QCamera(this);
    _videosink = new QVideoSink(this);
    _capture = new QMediaCaptureSession(this);
    connect(_videosink, SIGNAL(videoFrameChanged(QVideoFrame)), this, SLOT(frameCaptured(QVideoFrame)));
//    _frames = new Frames();
//    _frames->setCamera(_camera);
//    connect(_frames, SIGNAL(imageCaptured(QImage)), this, SLOT(cameraImgCaptured(QImage)));
    connect(this, SIGNAL(pushImg(QImage)), _sendImg, SLOT(ImageCaptured(QImage)));
    connect(_imgThread, SIGNAL(finished()), _sendImg, SLOT(clearImgQueue()));


    // 文本传输 包括创建、加入、退出、关闭摄像头信号
    _sendText   = new SendText();
    _textThread = new QThread();
    _sendText->moveToThread(_textThread);
    _sendText->start();
    _textThread->start();
    connect(this, SIGNAL(textReady(MSG_TYPE,QString)), _sendText, SLOT(pushText(MSG_TYPE,QString)));

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

    // 播放音效
    _soundEffect = new QSoundEffect(this);
    _soundEffect->setSource(QUrl::fromLocalFile(":/myEffect/resourse/2.wav"));

    // 滚动条
    ui->scrollArea->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { width:8px; background:rgba(0,0,0,0%); margin:0px,0px,0px,0px; padding-top:9px; padding-bottom:9px; } QScrollBar::handle:vertical { width:8px; background:rgba(0,0,0,25%); border-radius:4px; min-height:20; } QScrollBar::handle:vertical:hover { width:8px; background:rgba(0,0,0,50%); border-radius:4px; min-height:20; } QScrollBar::add-line:vertical { height:9px;width:8px; border-image:url(:/images/a/3.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical { height:9px;width:8px; border-image:url(:/images/a/1.png); subcontrol-position:top; } QScrollBar::add-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/4.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/2.png); subcontrol-position:top; } QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:rgba(0,0,0,10%); border-radius:4px; }");
    ui->listWidget->setStyleSheet("QScrollBar:vertical { width:8px; background:rgba(0,0,0,0%); margin:0px,0px,0px,0px; padding-top:9px; padding-bottom:9px; } QScrollBar::handle:vertical { width:8px; background:rgba(0,0,0,25%); border-radius:4px; min-height:20; } QScrollBar::handle:vertical:hover { width:8px; background:rgba(0,0,0,50%); border-radius:4px; min-height:20; } QScrollBar::add-line:vertical { height:9px;width:8px; border-image:url(:/images/a/3.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical { height:9px;width:8px; border-image:url(:/images/a/1.png); subcontrol-position:top; } QScrollBar::add-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/4.png); subcontrol-position:bottom; } QScrollBar::sub-line:vertical:hover { height:9px;width:8px; border-image:url(:/images/a/2.png); subcontrol-position:top; } QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:rgba(0,0,0,10%); border-radius:4px; }");
    vlayoutScroll = new QVBoxLayout(ui->scrollAreaWidgetContents);

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
//        QMessageBox::warning(this, "Connection error", _mytcpsocket->errorString() , QMessageBox::Yes, QMessageBox::Yes);
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
    QRegularExpression roomReg("^[1-9][0-9]{1, 8}$");
    if (!roomReg.match(roomNo).isValid())
        QMessageBox::warning(this, "RoomNo Error", "房间号不合法", QMessageBox::Yes, QMessageBox::Yes);
    else
        emit textReady(JOIN_MEETING, roomNo);
}

void Widget::on_btnExit_clicked()
{
//    if (_frames != nullptr && _frames->isCameraActive())
//        _frames->stopCam();
    if (_camera->isActive())
        _camera->stop();

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
    if (_camera->isActive())
    {
        _camera->stop();
        WRITE_LOG("stop camera");
        if (_camera->error() == QCamera::NoError)
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
        if (!initCamera())
        {
            QMessageBox::warning(this, "Camera Error", "找不到摄像头", QMessageBox::Yes, QMessageBox::Yes);
        }
//        _camera->start();
        WRITE_LOG("open camera");
        if (_camera->error() == QCamera::NoError)
        {
            _imgThread->start();
            ui->btnVideo->setText(CLOSEVIDEO);
        }
    }
}

bool Widget::initCamera()
{
    const auto settings = _camera->cameraDevice().videoFormats();
    if (settings.empty()) return false;
    auto s = settings.at(0);
    for (const auto &ss: settings)
    {
        if (ss.resolution().width() < s.resolution().width())
            s = ss;
    }

    _camera->setFocusMode(QCamera::FocusModeAuto);
    _camera->setCameraFormat(s);

    _capture->setCamera(_camera);
    _capture->setVideoSink(_videosink);

    _camera->start();
    return true;
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
    ChatMessage *message = new ChatMessage(ui->listWidget);
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

void Widget::textSend()
{
    qDebug() << "send text over";
    QListWidgetItem* lastItem = ui->listWidget->item(ui->listWidget->count() - 1);
    ChatMessage* messageW = (ChatMessage *)ui->listWidget->itemWidget(lastItem);
    messageW->setTextSuccess();
    ui->btnSend->setDisabled(false);
}

void Widget::frameCaptured(const QVideoFrame &_frame)
{
    auto frame = _frame;
//    frame.map(QVideoFrame::ReadOnly);
//    auto pf = QVideoFrameFormat::imageFormatFromPixelFormat(frame.pixelFormat());
//    auto pf = frame.toImage().format();
//    qDebug() << "img format: " << pf;
//    QPixmap pix;
//    auto ret = pix.loadFromData(frame.bits(0), frame.mappedBytes(0));
//    qDebug() << "ret: " << ret;
    if (frame.map(QVideoFrame::ReadOnly))
    {
//        auto databytes = frame.bits(0);
//        qDebug() << "mapped bytes: " << frame.mappedBytes(0);
//        auto videoImg = QImage(frame.bits(0), frame.width(), frame.height(), QVideoFrameFormat::imageFormatFromPixelFormat(frame.pixelFormat()));
//        auto videoImg = frame.toImage().convertToFormat(QImage::Format_RGB32);
        auto videoImg = frame.toImage();
//        if (videoImg.isNull()) qDebug() << "null img";
        frame.unmap();
        if (_partner.size() > 1)
        {
            emit pushImg(videoImg);
        }
        if (_mytcpsocket->getLocalIP() == mainIp)
        {
            ui->labMainWindow->setPixmap(QPixmap::fromImage(videoImg).scaled(ui->labMainWindow->size()));
        }

        auto *p = _partner[_mytcpsocket->getLocalIP()];
        if (p)
            p->setPic(videoImg);
    }
//    auto f = frame;
//    f.map(QVideoFrame::ReadOnly);
//    qDebug() << "is valid" << f.isValid();
//    qDebug() << "is Readable" << f.isReadable();

//    QImage videoImg = frame.toImage();
//    if (_partner.size() > 1)
//    {
//        emit pushImg(videoImg);
//    }
//    if (_mytcpsocket->getLocalIP() == mainIp)
//    {
//        ui->labMainWindow->setPixmap(QPixmap::fromImage(videoImg).scaled(ui->labMainWindow->size()));
//    }

//    auto *p = _partner[_mytcpsocket->getLocalIP()];
//    if (p)
//        p->setPic(videoImg);
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

void Widget::dataSolve(MESG *msg)
{
    if (msg->msg_type == CREATE_MEETING_RESPONSE)
    {
        int roomNo;
        memcpy(&roomNo, msg->data, msg->len);
        if (roomNo != 0)
        {
            QMessageBox::information(this, "Room No", QString("房间号：%1").arg(roomNo), QMessageBox::Yes, QMessageBox::Yes);

            ui->groupBoxMainWindow->setTitle(QString("主屏幕(房间号: %1)").arg(roomNo));
            ui->labOutLog->setText(QString("创建成功 房间号: %1").arg(roomNo) );
            WRITE_LOG("succeed creating room %d", roomNo);
            _createmeet = true;
            ui->btnExit->setDisabled(false);
            ui->btnVideo->setDisabled(false);
            ui->btnJoinMeeting->setDisabled(true);
            // add myself
            mainIp = _mytcpsocket->getLocalIP();
            addPartner(mainIp);
            ui->groupBoxMainWindow->setTitle(QHostAddress(mainIp).toString());
            ui->labMainWindow->setPixmap(QPixmap::fromImage(QImage(":/myImage/resourse/1.jpg").scaled(ui->labMainWindow->size())));
        }
        else
        {
            _createmeet = false;
            QMessageBox::information(this, "Room Information", QString("无可用房间"), QMessageBox::Yes, QMessageBox::Yes);
            ui->labOutLog->setText(QString("无可用房间"));
            ui->btnCreate->setDisabled(false);
            WRITE_LOG("no empty room");
        }
    }
    else if (msg->msg_type == JOIN_MEETING_RESPONSE)
    {
        qint32 c;
        memcpy(&c, msg->data, msg->len);
        if(c == 0)
        {
            QMessageBox::information(this, "Meeting Error", tr("会议不存在") , QMessageBox::Yes, QMessageBox::Yes);
            ui->labOutLog->setText(QString("会议不存在"));
            WRITE_LOG("meeting not exist");
            ui->btnExit->setDisabled(true);
            ui->btnVideo->setDisabled(true);
            ui->btnJoinMeeting->setDisabled(false);
            ui->btnConnect->setDisabled(true);
            _joinmeet = false;
        }
        else if(c == -1)
        {
            QMessageBox::warning(this, "Meeting information", "成员已满，无法加入" , QMessageBox::Yes, QMessageBox::Yes);
            ui->labOutLog->setText(QString("成员已满，无法加入"));
            WRITE_LOG("full room, cannot join");
        }
        else if (c > 0)
        {
            QMessageBox::warning(this, "Meeting information", "加入成功" , QMessageBox::Yes, QMessageBox::Yes);
            ui->labOutLog->setText(QString("加入成功"));
            WRITE_LOG("succeed joining room");
            //添加用户自己
            addPartner(_mytcpsocket->getLocalIP());
            mainIp = _mytcpsocket->getLocalIP();
            ui->groupBoxMainWindow->setTitle(QHostAddress(mainIp).toString());
            ui->labMainWindow->setPixmap(QPixmap::fromImage(QImage(":/myImage/resourse/1.jpg").scaled(ui->labMainWindow->size())));
            ui->btnJoinMeeting->setDisabled(true);
            ui->btnExit->setDisabled(false);
            ui->btnCreate->setDisabled(true);
            _joinmeet = true;
        }
    }
    else if (msg->msg_type == IMG_RECV)
    {
//        QHostAddress a(msg->ip);
//        qDebug() << a.toString();
        qDebug() << "img ip : " << QHostAddress(msg->ip).toString();
        QImage img;
        img.loadFromData(msg->data, msg->len);
        if (_partner.count(msg->ip) == 1)
        {
            auto *p = _partner[msg->ip];
            if (QDateTime::currentMSecsSinceEpoch() - p->lastClose > 5000)
                p->setPic(img);
        }
        else
        {
            auto *p = addPartner(msg->ip);
            p->setPic(img);
        }

        if (msg->ip == mainIp)
        {
            ui->labMainWindow->setPixmap(QPixmap::fromImage(img).scaled(ui->labMainWindow->size()));
        }
        repaint();
    }
    else if (msg->msg_type == TEXT_RECV)
    {
        QString str = QString::fromStdString(std::string((char *)msg->data, msg->len));
        //qDebug() << str;
        QString time = QString::number(QDateTime::currentDateTimeUtc().toSecsSinceEpoch());
        ChatMessage *message = new ChatMessage(ui->listWidget);
        QListWidgetItem *item = new QListWidgetItem();
        dealMessageTime(time);
        dealMessage(message, item, str, time, QHostAddress(msg->ip).toString() ,ChatMessage::User_she);
        if(str.contains('@' + QHostAddress(_mytcpsocket->getLocalIP()).toString()))
        {
            _soundEffect->play();
        }
    }
    else if (msg->msg_type == PARTNER_JOIN)
    {
        Partner* p = addPartner(msg->ip);
        if(p)
        {
            p->setPic(QImage(":/myImage/resourse/1.jpg"));
            ui->labOutLog->setText(QString("%1 join meeting").arg(QHostAddress(msg->ip).toString()));
            _iplist.append(QString("@") + QHostAddress(msg->ip).toString());
            ui->plainTextEdit->setCompleter(_iplist);
        }
    }
    else if (msg->msg_type == PARTNER_JOIN2)
    {
        uint32_t ip;
        int other = msg->len / sizeof(uint32_t), pos = 0;
        qDebug("num of others: %d", other);
        for (int i = 0; i < other; i++)
        {
            memcpy_s(&ip, sizeof(uint32_t), msg->data + pos , sizeof(uint32_t));
            qDebug() << "copy ip";
            pos += sizeof(uint32_t);
            Partner* p = addPartner(ip);
            if (p)
            {
                qDebug() << "start setting image";
                p->setPic(QImage(":/myImage/resourse/1.jpg"));
                _iplist << QString("@") + QHostAddress(ip).toString();
            }
            qDebug() << "set image";
        }
        qDebug() << "record ip tp iplist";
        ui->plainTextEdit->setCompleter(_iplist);
        ui->btnVideo->setDisabled(false);
    }
    else if (msg->msg_type == PARTNER_EXIT)
    {
        removePartner(msg->ip);
        if(mainIp == msg->ip)
        {
            ui->labMainWindow->setPixmap(QPixmap::fromImage(QImage(":/myImage/resourse/1.jpg").scaled(ui->labMainWindow->size())));
        }
        if(_iplist.removeOne(QString("@") + QHostAddress(msg->ip).toString()))
        {
            ui->plainTextEdit->setCompleter(_iplist);
        }
        else
        {
            qDebug() << QHostAddress(msg->ip).toString() << "not exist";
            WRITE_LOG("%s not exist",QHostAddress(msg->ip).toString().toStdString().c_str());
        }
        ui->labOutLog->setText(QString("%1 exit meeting").arg(QHostAddress(msg->ip).toString()));
    }
    else if (msg->msg_type == CLOSE_CAMERA)
    {
        qDebug() << "close camera handling...";
        closeImg(msg->ip);
    }
    else if (msg->msg_type == RemoteHostClosedError)
    {
        clearPartner();
        _mytcpsocket->disconnectFromHost();
        _mytcpsocket->wait();
        ui->labOutLog->setText(QString("关闭与服务器的连接"));
        ui->btnCreate->setDisabled(true);
        ui->btnExit->setDisabled(true);
        ui->btnConnect->setDisabled(false);
        ui->btnJoinMeeting->setDisabled(true);
        //清除聊天记录
        while(ui->listWidget->count() > 0)
        {
            QListWidgetItem *item = ui->listWidget->takeItem(0);
            ChatMessage *chat = (ChatMessage *)ui->listWidget->itemWidget(item);
            delete item;
            delete chat;
        }
        _iplist.clear();
        ui->plainTextEdit->setCompleter(_iplist);
        if(_createmeet || _joinmeet) QMessageBox::warning(this, "Meeting Information", "会议结束" , QMessageBox::Yes, QMessageBox::Yes);
    }
    else if (msg->msg_type == OtherNetError)
    {
        QMessageBox::warning(this, "Network Error", "网络异常" , QMessageBox::Yes, QMessageBox::Yes);
        clearPartner();
        _mytcpsocket->disconnectFromHost();
        _mytcpsocket->wait();
        ui->labOutLog->setText(QString("网络异常......"));
    }
    if (msg->data)
    {
        delete msg->data;
        msg->data = nullptr;
    }
    if (msg)
    {
        delete msg;
        msg = nullptr;
    }
}

Partner *Widget::addPartner(const quint32 ip)
{
//    qDebug() << "Widget::startPlay tid = " << QThread::currentThreadId();
    if (_partner.contains(ip)) return nullptr;
    Partner *p = new Partner(ui->scrollAreaWidgetContents, ip);
    connect(p, SIGNAL(sendIp(quint32)), this, SLOT(recvip(quint32)));
    _partner.insert(ip, p);
    vlayoutScroll->addWidget(p, 1);
    if (_partner.size() > 1)
    {
        connect(this, SIGNAL(volumnChange(int)), _ainput, SLOT(setVolumn(int)), Qt::UniqueConnection);
        connect(this, SIGNAL(volumnChange(int)), _aoutput, SLOT(setVolumn(int)), Qt::UniqueConnection);
        ui->btnAudio->setDisabled(false);
        ui->btnSend->setDefault(false);
        _aoutput->startPlay();
    }
    return p;
}

void Widget::removePartner(const quint32 ip)
{
    if (_partner.contains(ip))
    {
        auto *p = _partner[ip];
        disconnect(p, SIGNAL(sendIp(quint32)), this, SLOT(recvip(quint32)));
        vlayoutScroll->removeWidget(p);
        delete p;
        _partner.remove(ip);

        if (_partner.size() <= 1)
        {
            disconnect(_ainput, SLOT(setVolumn(int)));
            disconnect(_aoutput, SLOT(setVolumn(int)));
            _ainput->stopCollect();
            _aoutput->stopPlay();
            ui->btnAudio->setText(QString(OPENAUDIO).toUtf8());
            ui->btnAudio->setDisabled(true);
        }
    }
}

void Widget::clearPartner()
{
    ui->labMainWindow->setPixmap(QPixmap());
    if (_partner.size() == 0) return;
    auto iter = _partner.begin();
    while (iter != _partner.end())
    {
        auto ip = iter.key();
        iter++;
        auto *p = _partner.take(ip);
        vlayoutScroll->removeWidget(p);
        delete p;
        p = nullptr;
    }

    // close audio
    disconnect(_ainput, SLOT(setVolumn(int)));
    disconnect(_aoutput, SLOT(setVolumn(int)));
    _ainput->stopCollect();
    _aoutput->stopPlay();
    ui->btnAudio->setText(OPENAUDIO);
    ui->btnAudio->setDisabled(true);

    //close image sending thread
    if (_imgThread->isRunning())
    {
        _imgThread->quit();
        _imgThread->wait();
    }
    ui->btnVideo->setText(OPENVIDEO);
    ui->btnVideo->setDisabled(true);
}

void Widget::recvip(quint32 ip)
{
    if (_partner.contains(mainIp))
    {
        auto *p = _partner[mainIp];
        p->setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(0, 0 , 255, 0.7)");
    }
    if (_partner.contains(ip))
    {
        auto *p = _partner[mainIp];
        p->setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(255, 0 , 0, 0.7)");
    }
    ui->labMainWindow->setPixmap(QPixmap::fromImage(QImage(":/myImage/resourse/1.jpg").scaled(ui->labMainWindow->size())));
    mainIp = ip;
    auto str_ip = QHostAddress(mainIp).toString();
    ui->groupBoxMainWindow->setTitle(str_ip);
    qDebug() << str_ip;
}

void Widget::closeImg(const quint32 ip)
{
//    qDebug() << "_partners ip:";
//    foreach(quint32 _ip, _partner.keys()) {
//        qDebug() << QHostAddress(_ip).toString();
//    }
    auto str_ip = QHostAddress(ip).toString();
    qDebug() << "try to close camera of " + str_ip;
    if (!_partner.contains(ip))
    {
        qDebug() << "close camera error: can not find the user: " + str_ip;
        return;
    }
    auto *p = _partner[ip];
    p->setPic(QImage(":/myImage/resourse/1.jpg"));
    p->lastClose = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "peer camera closed";
    if (ip == mainIp)
        ui->labMainWindow->setPixmap(QPixmap::fromImage(QImage(":/myImage/resourse/1.jpg").scaled(ui->labMainWindow->size())));
}

void Widget::audioError(QString err)
{
    QMessageBox::warning(this, "Audio error", err, QMessageBox::Yes);
}

void Widget::speaks(const QString ip)
{
    ui->labOutLog->setText(QString(ip + "正在说话").toUtf8());
}

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    /*
     * 触发事件(3条， 一般使用第二条进行触发)
     * 1. 窗口部件第一次显示时，系统会自动产生一个绘图事件。从而强制绘制这个窗口部件，主窗口起来会绘制一次
     * 2. 当重新调整窗口部件的大小时，系统也会产生一个绘制事件--QWidget::update()或者QWidget::repaint()
     * 3. 当窗口部件被其它窗口部件遮挡，然后又再次显示出来时，就会对那些隐藏的区域产生一个绘制事件
    */
}

Widget::~Widget()
{
    //终止底层发送与接收线程

    if(_mytcpsocket->isRunning())
    {
        _mytcpsocket->stopImmedeiately();
        _mytcpsocket->wait();
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

