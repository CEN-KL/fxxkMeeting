#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H
#include <QObject>
#include <QThread>
#include <QAudioSink>
#include <QMutex>

class AudioOutput : public QThread
{
    Q_OBJECT
public:
    AudioOutput(QObject *par = nullptr);
    ~AudioOutput();
    void stopImmediately();
    void startPlay();
    void stopPlay();
private:
    QAudioSink *audiosink;
    QIODevice *outputdevice;
    QMutex device_lock;
    QByteArray m_pcmDataBuffer;

    volatile bool isCanRun;
    QMutex m_lock;
    void run() override;
    QString errorString();
private slots:
    void handleStateChanged(QAudio::State);
    void setVolumn(int);
    void clearQueue();
signals:
    void audioOutputError(QString);
    void speaker(QString);
};

#endif // AUDIOOUTPUT_H
