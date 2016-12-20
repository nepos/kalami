#include "wpasupplicantbss.h"

WpaSupplicantBss::WpaSupplicantBss(const QDBusObjectPath &path, QObject *parent) :
    QObject(parent), path(path)
{
    dbus = new QDBusInterface("fi.w1.wpa_supplicant1",
                              path.path(),
                              "fi.w1.wpa_supplicant1.BSS",
                              QDBusConnection::systemBus(), this);
}

bool WpaSupplicantBss::operator ==(const WpaSupplicantBss &other)
{
    return dbus->path() == other.dbus->path();
}

WpaSupplicantBss::~WpaSupplicantBss()
{
    delete dbus;
}
