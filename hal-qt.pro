QT += core websockets dbus
QT -= gui

CONFIG += c++11

TARGET = hal-qt
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    daemon.cpp \
    inputdevice.cpp \
    udevdevice.cpp \
    udevmonitor.cpp \
    connmanagentadaptor.cpp \
    connmanagent.cpp

HEADERS += \
    daemon.h \
    inputdevice.h \
    udevdevice.h \
    udevmonitor.h \
    types.h \
    connmanagentadaptor.h \
    connmanagent.h

LIBS += -ludev

DISTFILES += \
    io.nepos.connman.Agent
