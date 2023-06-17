#include "audiooutput.h"
#include "netheader.h"
#include <QMediaDevices>
#include <QHostAddress>

const quint32 FRAME_LEN_125MS = 1900;

extern QUEUE_DATA<MESG> audio_recv;

AudioOutput::AudioOutput(QObject *par): QThread(par)
{
    QAudioFormat format;
    // set format
    format.setSampleRate(16000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!device.isFormatSupported(format))
    {
        qWarning("Default format not supported, trying to use the preferred format.");
        format = QMediaDevices::defaultAudioOutput().preferredFormat();
    }
    audiosink = new QAudioSink(format, this);
    connect(audiosink, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    outputdevice = nullptr;
}

AudioOutput::~AudioOutput()
{
    delete audiosink;
}

QString AudioOutput::errorString()
{
    if (audiosink->error() == QAudio::OpenError)
    {
        return QString("AudioOutput An error occurred opening the audio device").toUtf8();
    }
    else if (audiosink->error() == QAudio::IOError)
    {
        return QString("AudioOutput An error occurred during read/write of audio device").toUtf8();
    }
    else if (audiosink->error() == QAudio::UnderrunError)
    {
        return QString("AudioOutput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();
    }
    else if (audiosink->error() == QAudio::FatalError)
    {
        return QString("AudioOutput A non-recoverable error has occurred, the audio device is not usable at this time.");
    }
    else
    {
        return QString("AudioOutput No errors have occurred").toUtf8();
    }
}

void AudioOutput::run()
{
    isCanRun = true;
    QByteArray m_pcmDataBuffer;

    WRITE_LOG("start palying audio thread 0x%p", QThread::currentThreadId());
    while (true)
    {
        {
            QMutexLocker lock(&m_lock);
            if (!isCanRun)
            {
                stopPlay();
                WRITE_LOG("stop playing audio thread 0x%p", QThread::currentThread());
                return;
            }
        }
        MESG *msg = audio_recv.pop_msg();
        if (msg == nullptr) continue;
        {
            QMutexLocker locker(&m_lock);
            if (outputdevice != nullptr)
            {
                m_pcmDataBuffer.append((char *)msg->data, msg->len);

                if (m_pcmDataBuffer.size() >= FRAME_LEN_125MS)
                {
                    qint64 ret = outputdevice->write(m_pcmDataBuffer.data(), FRAME_LEN_125MS);
                    if (ret < 0)
                    {
                        qDebug() << outputdevice->errorString();
                        return;
                    }
                    else
                    {
                        emit speaker(QHostAddress(msg->ip).toString());
                        m_pcmDataBuffer = m_pcmDataBuffer.right(m_pcmDataBuffer.size() - ret);
                    }
                }
            }
            else
            {
                m_pcmDataBuffer.clear();
            }
        }
        if (msg->data) delete msg->data;
        if (msg) delete msg;
    }
}

void AudioOutput::stopImmediately()
{
    QMutexLocker locker(&m_lock);
    isCanRun = false;
}

void AudioOutput::setVolumn(int v)
{
    audiosink->setVolume(v / 100.0);
}

void AudioOutput::clearQueue()
{
    qDebug() << "audio_recv clear";
    audio_recv.clear();
}

void AudioOutput::startPlay()
{
    if (audiosink->state() == QAudio::ActiveState) return;
    WRITE_LOG("start playing audio");
    outputdevice = audiosink->start();
}

void AudioOutput::stopPlay()
{
    if (audiosink->state() == QAudio::StoppedState) return;
    {
        QMutexLocker lock(&device_lock);
        outputdevice = nullptr;
    }
    audiosink->stop();
    WRITE_LOG("stop playing audio");
}

void AudioOutput::handleStateChanged(QAudio::State newState)
{
    switch(newState)
    {
    case QAudio::ActiveState:
        break;
    case QAudio::SuspendedState:
        break;
    case QAudio::StoppedState:
        if (audiosink->error() != QAudio::NoError)
        {
            audiosink->stop();
            emit audioOutputError(errorString());
            qDebug() << "out audio error";
        }
        break;
    case QAudio::IdleState:
        break;
    default:
        break;
    }
}
