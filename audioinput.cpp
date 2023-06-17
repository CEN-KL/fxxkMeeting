#include "audioinput.h"
#include "netheader.h"
#include <QAudioFormat>
#include <QDebug>
#include <QThread>
#include <QMediaDevices>

extern QUEUE_DATA<MESG> queue_send;
extern QUEUE_DATA<MESG> queue_recv;

AudioInput::AudioInput(QObject *par) : QObject(par)
{
    buf = new char[2 * MB];
    QAudioFormat format;
    // set format
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (!device.isFormatSupported(format))
    {
        qWarning("Default format not supported, trying to use the preferred format.");
        format = QMediaDevices::defaultAudioInput().preferredFormat();
    }
    audiosource = new QAudioSource(format, this);
    connect(audiosource, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));

}

AudioInput::~AudioInput()
{
    delete audiosource;
}

void AudioInput::startCollect()
{
    if (audiosource->state() == QAudio::ActiveState) return;
    WRITE_LOG("start collecting audio");
    inputdevice = audiosource->start();
    connect(inputdevice, SIGNAL(readyRead()), this, SLOT(onreadyRead()));
}

void AudioInput::stopCollect()
{
    if (audiosource->state() == QAudio::StoppedState) return;
    disconnect(this, SLOT(onreadyRead()));
    audiosource->stop();
    WRITE_LOG("stop collecting audio");
    inputdevice = nullptr;
}


void AudioInput::onreadyRead()
{
    static int num = 0, total = 0;
    if (inputdevice == nullptr) return;
    int len = inputdevice->read(buf + total, 2 * MB - total);
    if (num < 2)
    {
        total += len;
        num++;
        return;
    }
    total += len;
    qDebug() << "total_len = " << total;
    MESG *msg = new MESG();
    memset(msg, 0, sizeof(MESG));
    msg->msg_type = AUDIO_SEND;
    QByteArray rr(buf, total);
    QByteArray cc = qCompress(rr).toBase64();
    msg->len = cc.size();

    msg->data = new uchar[msg->len];
    memset(msg->data, 0, msg->len);
    memcpy_s(msg->data, msg->len, cc.data(), cc.size());
    queue_send.push_msg(msg);
    total = 0;
    num = 0;
}

QString AudioInput::errorString()
{
    if (audiosource->error() == QAudio::OpenError)
    {
        return QString("AudioInput An error occurred opening the audio device").toUtf8();
    }
    else if (audiosource->error() == QAudio::IOError)
    {
        return QString("AudioInput An error occurred during read/write of audio device").toUtf8();
    }
    else if (audiosource->error() == QAudio::UnderrunError)
    {
        return QString("AudioInput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();
    }
    else if (audiosource->error() == QAudio::FatalError)
    {
        return QString("AudioInput A non-recoverable error has occurred, the audio device is not usable at this time.");
    }
    else
    {
        return QString("AudioInput No errors have occurred").toUtf8();
    }
}

void AudioInput::handleStateChanged(QAudio::State newState)
{
    switch (newState)
    {
    case QAudio::StoppedState:
        if (audiosource->error() != QAudio::NoError)
        {
            stopCollect();
            emit audioInputError(errorString());
        }
        else
        {
            qWarning("stop recording");
        }
        break;
    case QAudio::ActiveState:
        qWarning("start recording");
        break;
    default:
        break;
    }
}

void AudioInput::setVolumn(int v)
{
    qDebug() << v;
    audiosource->setVolume(v / 100.0);
}
