#include <QDebug>
#include <QMetaObject>
#include <QMetaMethod>

#include "wpasupplicantbss.h"

WpaSupplicantBss::WpaSupplicantBss(const QDBusObjectPath &path, QObject *parent) :
    QObject(parent), path(path)
{
    dbus = new QDBusInterface("fi.w1.wpa_supplicant1",
                              path.path(),
                              "fi.w1.wpa_supplicant1.BSS",
                              QDBusConnection::systemBus(), this);

    if (!dbus->isValid())
        return;

    ssid = dbus->property("SSID").toByteArray();

    // 0x617b73767d = "a{sv}"
    QObject::connect(dbus, SIGNAL(PropertiesChanged(QDBusRawType<0x617b73767d>*)), this, SLOT(propertiesChanged()));

    propertiesChanged();
}

void WpaSupplicantBss::propertiesChanged()
{
    setProperty("signal", dbus->property("Signal"));
    setProperty("age", dbus->property("Age"));
    //qDebug() << "properties of bss changed. ssid" << ssid << "signal" << signal << "path" << dbus->path();
}

bool WpaSupplicantBss::operator ==(const WpaSupplicantBss &other)
{
    return dbus->path() == other.dbus->path();
}

WpaSupplicantBss::~WpaSupplicantBss()
{
    delete dbus;
}
