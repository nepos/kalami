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
    dbus_fi.w1.wpa_supplicant1.cpp \
    dbus_fi.w1.wpa_supplicant1.Interface.cpp \
    dbus_org.freedesktop.DBus.cpp \
    dbus_fi.w1.wpa_supplicant1.Network.cpp \
    dbus_fi.w1.wpa_supplicant1.BSS.cpp \
    wifimanager.cpp

HEADERS += \
    daemon.h \
    inputdevice.h \
    udevdevice.h \
    udevmonitor.h \
    dbus_fi.w1.wpa_supplicant1.h \
    dbus_fi.w1.wpa_supplicant1.Interface.h \
    dbus_org.freedesktop.DBus.h \
    types.h \
    dbus_fi.w1.wpa_supplicant1.Network.h \
    dbus_fi.w1.wpa_supplicant1.BSS.h \
    wifimanager.h

LIBS += -ludev

DISTFILES += \
    fi.w1.wpa_supplicant1.xml \
    fi.w1.wpa_supplicant1.Interface.xml \
    org.freedesktop.DBus.xml \
    fi.w1.wpa_supplicant1.Network.xml \
    fi.w1.wpa_supplicant1.BSS.xml
