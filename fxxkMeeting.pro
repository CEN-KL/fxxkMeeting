QT       += core gui
QT       += network
QT       += multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    audioinput.cpp \
    audiooutput.cpp \
    chatmessage.cpp \
    logqueue.cpp \
    main.cpp \
    mytcpsocket.cpp \
    mytextedit.cpp \
    netheader.cpp \
    partner.cpp \
    recvsolve.cpp \
    screen.cpp \
    sendimg.cpp \
    sendtext.cpp \
    widget.cpp

HEADERS += \
    audioinput.h \
    audiooutput.h \
    chatmessage.h \
    logqueue.h \
    mytcpsocket.h \
    mytextedit.h \
    netheader.h \
    partner.h \
    recvsolve.h \
    screen.h \
    sendimg.h \
    sendtext.h \
    widget.h

FORMS += \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc
