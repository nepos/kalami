QT += core websockets
QT -= gui

CONFIG += c++11

TARGET = hal-qt
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    daemon.cpp \
    inputdevice.cpp

HEADERS += \
    daemon.h \
    inputdevice.h
