QT += core websockets dbus network nfc
QT -= gui

CONFIG += c++11

TARGET = kalami
CONFIG += console
CONFIG -= app_bundle

DEFINES += QT_NO_DEBUG_OUTPUT

TEMPLATE = app

SOURCES += main.cpp \
    daemon.cpp \
    inputdevice.cpp \
    connman.cpp \
    updater.cpp \
    alsamixer.cpp \
    machine.cpp \
    gptparser.cpp \
    i2cclient.cpp \
    fring.cpp \
    nfc.cpp \
    gpio.cpp \
    polyphantmessage.cpp \
    polyphantconnection.cpp \
    brightnesscontrol.cpp \
    imagereader.cpp \
    blockdevice.cpp

HEADERS += \
    daemon.h \
    inputdevice.h \
    connman.h \
    updater.h \
    alsamixer.h \
    machine.h \
    gptparser.h \
    i2cclient.h \
    fring.h \
    nfc.h \
    gpio.h \
    polyphantmessage.h \
    polyphantconnection.h \
    brightnesscontrol.h \
    fring-protocol.h \
    imagereader.h \
    crc32table.h \
    blockdevice.h

LIBS += -ludev
LIBS += -lqconnman
LIBS += -lasound
LIBS += -lz -lvcddec -lvcdcom

