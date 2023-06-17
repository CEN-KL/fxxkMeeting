#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QObject>
#include <QIODevice>
#include <QAudioInput>
#include <QAudioSource>

class AudioInput : public QObject
{
    Q_OBJECT
public:
    AudioInput(QObject *par = nullptr);
    ~AudioInput();
private:
    QAudioSource *audiosource;
    QIODevice* inputdevice;
    char *buf;
private slots:
    void onreadyRead();
    void handleStateChanged(QAudio::State);
    QString errorString();
    void setVolumn(int);
public slots:
    void startCollect();
    void stopCollect();
signals:
    void audioInputError(QString);
};

#endif // AUDIOINPUT_H
