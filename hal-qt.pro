QT += core websockets dbus
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
    connman.cpp

HEADERS += \
    daemon.h \
    reduxproxy.h \
    inputdevice.h \
    udevdevice.h \
    udevmonitor.h \
    types.h \
    connman.h

LIBS += -ludev
LIBS += -lqconnman

DISTFILES += \
    io.nepos.connman.Agent
