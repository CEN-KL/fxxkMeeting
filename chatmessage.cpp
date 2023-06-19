#include "chatmessage.h"
#include <QMovie>
#include <QDateTime>
#include <QDebug>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>

ChatMessage::ChatMessage(QWidget *par): QWidget(par)
{
    QFont text_font = this->font();
    text_font.setFamily("Fira Code");
    text_font.setPointSize(12);
    m_leftPixmap = QPixmap(":/myImage/resourse/1.jpg");
    m_rightPixmap = QPixmap(":/myImage/resourse/1.jpg");

    m_loadingMovie = new QMovie(this);
    m_loadingMovie->setFileName(":/myEffect/resourse/3.gif");
    m_loading = new QLabel(this);
    m_loading->setMovie(m_loadingMovie);
    m_loading->setScaledContents(true);
    m_loading->resize(40, 40);
    m_loading->setAttribute(Qt::WA_TranslucentBackground, true);  //背景透明
}

void ChatMessage::setTextSuccess()
{
    m_loading->hide();
    m_loadingMovie->stop();
    m_isSending = true;
}

void ChatMessage::setText(QString text, QString time, QSize size, QString ip, user_type ut)
{
    m_msg = text;
    m_userType = ut;
    m_time = time;
    m_curTime = QDateTime::fromSecsSinceEpoch(time.toInt()).toString("ddd hh:mm");
    m_allsize = size;
    m_ip = ip;

    if (ut == User_me)
    {
        if (!m_isSending)
        {
            m_loading->move(m_boxRightRect.x() - m_loading->width() - 10,
                            m_boxRightRect.y() + m_boxRightRect.height()/2 - m_loading->height()/2);
            m_loading->show();
            m_loadingMovie->start();
        }
        else
        {
            m_loading->hide();
        }
    }
    this->update();
}

QSize ChatMessage::fontRect(QString str)
{
    m_msg = str;
    int minHeight = 30;
    int iconWidth = 40;
    int iconSpaceWidth = 20;
    int iconRectWidth = 5;
    int iconTMPH = 10;
    int triangelWidth = 6;
    int boxTMP = 20;
    int textSpaceRect = 12;

    m_boxWidth = this->width() - boxTMP - 2 * (iconWidth + iconSpaceWidth + iconRectWidth);
    m_textWidth = m_boxWidth - 2 * textSpaceRect; // 文字消息块宽度
    m_spaceWidth = this->width() - m_textWidth;
    m_iconLeftRect  = QRect(iconSpaceWidth, iconTMPH + 10, iconWidth, iconWidth);
    m_iconRightRect = QRect(this->width() - iconSpaceWidth - iconWidth, iconTMPH + 10
                            , iconWidth, iconWidth);

    QSize size = getRealString(m_msg); // 整体size

    qDebug() << "fontRect size: " << size;
    int height = qMax(minHeight, size.height());

    m_triangelLeftRect  = QRect(iconWidth + iconSpaceWidth + iconRectWidth, m_lineHeight/2 + 10,
                                triangelWidth, height - m_lineHeight);
    m_triangelRightRect = QRect(this->width() - iconRectWidth - iconWidth - iconSpaceWidth - triangelWidth, m_lineHeight/2 + 10,
                                triangelWidth, height - m_lineHeight);

    if (size.width() < m_textWidth + m_spaceWidth)
    {
        m_boxLeftRect.setRect(m_triangelLeftRect.x() + m_triangelLeftRect.width(), m_lineHeight/4 * 3 + 10,
                              size.width() - m_spaceWidth + 2 * textSpaceRect, height - m_lineHeight);
        m_boxRightRect.setRect(this->width() - size.width() + m_spaceWidth - 2 * textSpaceRect - iconWidth - iconSpaceWidth - iconRectWidth - triangelWidth, m_lineHeight/4 * 3 + 10,
                               size.width() - m_spaceWidth + 2 * textSpaceRect, height - m_lineHeight);
    } else
    {
        m_boxLeftRect.setRect(m_triangelLeftRect.x() + m_triangelLeftRect.width(), m_lineHeight/4 * 3 + 10,
                              m_boxWidth, height - m_lineHeight);
        m_boxRightRect.setRect(iconWidth + boxTMP + iconSpaceWidth + iconRectWidth - triangelWidth, m_lineHeight/4 * 3 + 10,
                               m_boxWidth, height - m_lineHeight);
    }

    m_textLeftRect.setRect(m_boxLeftRect.x() + textSpaceRect, m_boxLeftRect.y() + iconTMPH,
                           m_boxLeftRect.width() - 2 * textSpaceRect, m_boxLeftRect.height() - 2 * iconTMPH);
    m_textRightRect.setRect(m_boxRightRect.x() + textSpaceRect, m_boxRightRect.y() + iconTMPH,
                            m_boxRightRect.width() - 2 * textSpaceRect, m_boxRightRect.height() - 2 * iconTMPH);

    m_ipLeftRect.setRect(m_boxLeftRect.x(), m_boxLeftRect.y() + iconTMPH - 20,
                         m_boxLeftRect.width() - 2 * textSpaceRect + iconWidth * 2, 20);
    m_ipRightRect.setRect(m_boxRightRect.x(), m_boxRightRect.y() + iconTMPH - 30,
                          m_boxRightRect.width() - 2 * textSpaceRect + iconWidth * 2 , 20);

    return QSize(size.width(), height + 15);
}

QSize ChatMessage::getRealString(QString src)
{
    QFontMetricsF fm(this->font());
    m_lineHeight = fm.lineSpacing(); // 获取行高
    int nCount = src.count("\n");
    int nMaxWidth = 0;
    if(nCount == 0) {
        nMaxWidth = fm.horizontalAdvance(src);
        QString value = src;
        if(nMaxWidth > m_textWidth) {
            // 如果字符串宽度超过预设的 m_textWidth 就要进行换行处理 ; 换行符个数num
            nMaxWidth = m_textWidth;
            int size = m_textWidth / fm.horizontalAdvance(" ");
            int num = fm.horizontalAdvance(value) / m_textWidth;
            num = ( fm.horizontalAdvance(value) ) / m_textWidth;
            nCount += num;
            QString temp = "";
            for(int i = 0; i < num; i++) {
                temp += value.mid(i*size, (i+1)*size) + "\n";
            }
            src.replace(value, temp);
        }
    } else {
        for(int i = 0; i < (nCount + 1); i++) {
            QString value = src.split("\n").at(i);
            nMaxWidth = fm.horizontalAdvance(value) > nMaxWidth ? fm.horizontalAdvance(value) : nMaxWidth;
            if(fm.horizontalAdvance(value) > m_textWidth) {
                nMaxWidth = m_textWidth;
                int size = m_textWidth / fm.horizontalAdvance(" ");
                int num = fm.horizontalAdvance(value) / m_textWidth;
                num = ((i + num) * fm.horizontalAdvance(" ") + fm.horizontalAdvance(value)) / m_textWidth;
                nCount += num;
                QString temp = "";
                for(int i = 0; i < num; i++) {
                    temp += value.mid(i * size, (i + 1) * size) + "\n";
                }
                src.replace(value, temp);
            }
        }
    }
    return QSize(nMaxWidth + m_spaceWidth, (nCount + 1) * m_lineHeight + 2 * m_lineHeight);
}

void ChatMessage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);//消锯齿
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(Qt::gray));

    if(m_userType == user_type::User_she) { // 用户
        //头像
        //        painter.drawRoundedRect(m_iconLeftRect,m_iconLeftRect.width(),m_iconLeftRect.height());
        painter.drawPixmap(m_iconLeftRect, m_leftPixmap);

        //框加边
        QColor col_boxB(234, 234, 234);
        painter.setBrush(QBrush(col_boxB));
        painter.drawRoundedRect(m_boxLeftRect.x() - 1, m_boxLeftRect.y() - 1 + 10, m_boxLeftRect.width() + 2, m_boxLeftRect.height() + 2, 4, 4);
        //框
        QColor col_box(255, 255, 255);
        painter.setBrush(QBrush(col_box));
        painter.drawRoundedRect(m_boxLeftRect, 4, 4);

        //三角
        QPointF points[3] = {
            QPointF(m_triangelLeftRect.x(), 40),
            QPointF(m_triangelLeftRect.x() + m_triangelLeftRect.width(), 35),
            QPointF(m_triangelLeftRect.x() + m_triangelLeftRect.width(), 45),
        };
        QPen pen;
        pen.setColor(col_box);
        painter.setPen(pen);
        painter.drawPolygon(points, 3);

        //三角加边
        QPen pentriangelBian;
        pentriangelBian.setColor(col_boxB);
        painter.setPen(pentriangelBian);
        painter.drawLine(QPointF(m_triangelLeftRect.x() - 1, 30), QPointF(m_triangelLeftRect.x()+m_triangelLeftRect.width(), 24));
        painter.drawLine(QPointF(m_triangelLeftRect.x() - 1, 30), QPointF(m_triangelLeftRect.x()+m_triangelLeftRect.width(), 36));

        //ip
        //ip
        QPen penIp;
        penIp.setColor(Qt::darkGray);
        painter.setPen(penIp);
        QFont f = this->font();
        f.setPointSize(10);
        QTextOption op(Qt::AlignHCenter | Qt::AlignVCenter);
        painter.setFont(f);
        painter.drawText(m_ipLeftRect, m_ip, op);

        //内容
        QPen penText;
        penText.setColor(QColor(51, 51, 51));
        painter.setPen(penText);
        QTextOption option(Qt::AlignLeft | Qt::AlignVCenter);
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        painter.setFont(this->font());
        painter.drawText(m_textLeftRect, m_msg, option);
    }  else if(m_userType == user_type::User_me) { // 自己
        //头像
        //        painter.drawRoundedRect(m_iconRightRect,m_iconRightRect.width(),m_iconRightRect.height());
        painter.drawPixmap(m_iconRightRect, m_rightPixmap);

        //框
        QColor col_box(75, 164, 242);
        painter.setBrush(QBrush(col_box));
        painter.drawRoundedRect(m_boxRightRect, 4, 4);

        //三角
        QPointF points[3] = {
            QPointF(m_triangelRightRect.x() + m_triangelRightRect.width(), 40),
            QPointF(m_triangelRightRect.x(), 35),
            QPointF(m_triangelRightRect.x(), 45),
        };
        QPen pen;
        pen.setColor(col_box);
        painter.setPen(pen);
        painter.drawPolygon(points, 3);


        //ip
        QPen penIp;
        penIp.setColor(Qt::black);
        painter.setPen(penIp);
        QFont f = this->font();
        f.setPointSize(10);
        QTextOption op(Qt::AlignHCenter | Qt::AlignVCenter);
        painter.setFont(f);
        painter.drawText(m_ipRightRect, m_ip, op);

        //内容
        QPen penText;
        penText.setColor(Qt::white);
        painter.setPen(penText);
        QTextOption option(Qt::AlignLeft | Qt::AlignVCenter);
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        painter.setFont(this->font());
        painter.drawText(m_textRightRect, m_msg, option);
    }  else if(m_userType == user_type::User_time) { // 时间
        QPen penText;
        penText.setColor(QColor(153, 153, 153));
        painter.setPen(penText);
        QTextOption option(Qt::AlignCenter);
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        QFont te_font = this->font();
        te_font.setFamily("MicrosoftYaHei");
        te_font.setPointSize(10);
        painter.setFont(te_font);
        painter.drawText(this->rect(), m_curTime, option);
    }
};
