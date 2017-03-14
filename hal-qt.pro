QT += core websockets dbus network
QT -= gui

CONFIG += c++11

TARGET = hal-qt
CONFIG += console
CONFIG -= app_bundle

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
    machine.cpp

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
    machine.h

LIBS += -ludev
LIBS += -lqconnman
LIBS += -lasound

DISTFILES += \
    io.nepos.connman.Agent
