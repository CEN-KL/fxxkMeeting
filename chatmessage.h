#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QWidget>
#include <QLabel>

class ChatMessage : public QWidget
{
    Q_OBJECT
public:
    explicit ChatMessage(QWidget *par = nullptr);

    enum user_type
    {
        User_system,
        User_me,
        User_she,
        User_time,
    };

    void setTextSuccess();
    void setText(QString text, QString time, QSize size, QString ip = "", user_type uc = User_time);

    QSize getRealString(QString src);
    QSize fontRect(QString str);

    inline QString text() { return m_msg; }
    inline QString time() { return m_time; }
    inline user_type userType() { return m_userType; }

protected:
    void paintEvent(QPaintEvent *) override;
private:
    QString m_msg;
    QString m_time;
    QString m_curTime;
    QString m_ip;

    QSize m_allsize;
    user_type m_userType;

    int m_boxWidth;
    int m_textWidth;
    int m_spaceWidth;
    int m_lineHeight;

    QRect m_ipLeftRect;
    QRect m_ipRightRect;

    QRect m_iconLeftRect;
    QRect m_iconRightRect;

    QRect m_triangelLeftRect;
    QRect m_triangelRightRect;

    QRect m_boxLeftRect;
    QRect m_boxRightRect;

    QRect m_textLeftRect;
    QRect m_textRightRect;

    QPixmap m_leftPixmap;
    QPixmap m_rightPixmap;

    QLabel *m_loading = Q_NULLPTR;
    QMovie *m_loadingMovie = Q_NULLPTR;
    bool m_isSending = false;
};

#endif // CHATMESSAGE_H
