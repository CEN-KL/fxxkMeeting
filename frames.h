#ifndef FRAMES_H
#define FRAMES_H

#include <QVideoSink>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QPointer>


class Frames : public QVideoSink
{
    Q_OBJECT
public:
    explicit Frames(QObject *par = nullptr);
    ~Frames() override;
    void setVideoSink( QVideoSink * newVideoSink );
    QVideoSink * videoSink() const;
    void stopCam();
    inline bool isCameraActive() const { return m_cam->isActive(); }
    inline QCamera::Error cameraError() const { return m_cam->error(); }

private:
    Q_DISABLE_COPY( Frames )
    QCamera * m_cam;
    QMediaCaptureSession m_capture;
    QPointer<QVideoSink> m_videoSink;
    QString m_formatString;
    QString m_fpsString;
    int m_fps;
    QTimer * m_1s;

private slots:
    void newFrame(const QVideoFrame &frame);
    void initCam();
    void timer();
signals:
    void imageCaptured(const QImage &);
    void formatStringChanged();
    void fpsStringChanged();
    void videoSinkChanged();
};

#endif // FRAMES_H
