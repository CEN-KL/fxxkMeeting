#ifndef PARTNER_H
#define PARTNER_H
#include <QLabel>
#include <QImage>


class Partner : public QLabel
{
    Q_OBJECT
public:
    Partner(QWidget * par, quint32 = 0);
    void setPic(QImage);
    qint64 lastClose = 0;
private:
    quint32 ip;

    void mousePressEvent(QMouseEvent *) override;
    int w;
signals:
    void sendIp(quint32);
};

#endif // PARTNER_H
