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
    wpasupplicant.cpp \
    wpasupplicantinterface.cpp \
    wpasupplicantnetwork.cpp \
    wpasupplicantbss.cpp

HEADERS += \
    daemon.h \
    inputdevice.h \
    udevdevice.h \
    udevmonitor.h \
    wpasupplicant.h \
    wpasupplicantinterface.h \
    wpasupplicantnetwork.h \
    wpasupplicantbss.h

LIBS += -ludev
