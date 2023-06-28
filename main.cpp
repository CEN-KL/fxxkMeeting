#include "widget.h"
#include "screen.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Screen::init();

//    qDebug() << "main tid = " << QThread::currentThreadId();
    Widget w;
    w.show();
    return a.exec();
}
