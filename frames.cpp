#include "frames.h"
#include <QTimer>
#include <QVideoFrame>
#include <QCameraDevice>

Frames::Frames(QObject *par)
    : QVideoSink(par)
    , m_cam(nullptr)
    , m_fps(0)
    , m_1s(new QTimer(this))
{
    connect(m_1s, &QTimer::timeout, this, &Frames::timer);
    connect(this, &QVideoSink::videoFrameChanged, this, &Frames::newFrame);
    m_1s->setInterval(100);
    m_1s->start();

    initCam();
}

Frames::~Frames()
{
    stopCam();
}

void Frames::newFrame(const QVideoFrame &frame)
{
    QVideoFrame f = frame;
    f.map(QVideoFrame::ReadOnly);
    if (f.isValid())
    {
        QImage img = f.toImage();
        f.unmap();
        emit imageCaptured(img);
    }
    ++m_fps;
    if (m_videoSink)
        m_videoSink->setVideoFrame(frame);
}

void Frames::initCam()
{
    m_cam = new QCamera(this);
    const auto settings = m_cam->cameraDevice().videoFormats();
    auto s = settings.at(0);
    for (const auto &ss: settings)
    {
        if (ss.resolution().width() < s.resolution().width())
            s = ss;
    }

    m_cam->setFocusMode(QCamera::FocusModeAuto);
    m_cam->setCameraFormat(s);

    m_capture.setCamera(m_cam);
    m_capture.setVideoSink(this);

    m_formatString = QString( "%1x%2 at %3 fps, %4" ).arg( QString::number( s.resolution().width() ),
                                                        QString::number( s.resolution().height() ), QString::number( (int) s.maxFrameRate() ),
                                                        QVideoFrameFormat::pixelFormatToString( s.pixelFormat() ) );
    emit formatStringChanged();
    m_cam->start();
}

void Frames::stopCam()
{
    m_cam->stop();

    disconnect( m_cam, 0, 0, 0 );
    m_cam->setParent( nullptr );

    delete m_cam;

    m_cam = nullptr;
}

QVideoSink * Frames::videoSink() const
{
    return m_videoSink.get();
}

void Frames::setVideoSink( QVideoSink * newVideoSink )
{
    if( m_videoSink == newVideoSink )
        return;

    m_videoSink = newVideoSink;

    emit videoSinkChanged();
}

void Frames::timer()
{
    m_fpsString = QString( "%1 fps" ).arg( QString::number( m_fps ) );
    m_fps = 0;
    emit fpsStringChanged();
}
