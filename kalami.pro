QT += core websockets dbus network
QT -= gui

CONFIG += c++11

TARGET = kalami
CONFIG += console
CONFIG -= app_bundle

DEFINES += QT_NO_DEBUG_OUTPUT

TEMPLATE = app

SOURCES += main.cpp \
    daemon.cpp \
    reduxproxy.cpp \
    inputdevice.cpp \
    udevdevice.cpp \
    udevmonitor.cpp \
    connman.cpp \
    linuxled.cpp \
    updater.cpp \
    alsamixer.cpp \
    machine.cpp \
    gptparser.cpp \
    i2cclient.cpp \
    fring.cpp

HEADERS += \
    daemon.h \
    reduxproxy.h \
    inputdevice.h \
    udevdevice.h \
    udevmonitor.h \
    connman.h \
    linuxled.h \
    updater.h \
    alsamixer.h \
    machine.h \
    gptparser.h \
    i2cclient.h \
    fring.h

LIBS += -ludev
LIBS += -lqconnman
LIBS += -lasound
