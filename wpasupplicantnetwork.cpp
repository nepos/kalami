#include "wpasupplicantnetwork.h"

WpaSupplicantNetwork::WpaSupplicantNetwork(const QDBusObjectPath &path, QObject *parent) :
    QObject(parent)
{
    dbus = new QDBusInterface("fi.w1.wpa_supplicant1",
                              path.path(),
                              "fi.w1.wpa_supplicant1.Network",
                              QDBusConnection::systemBus(), this);
}

bool WpaSupplicantNetwork::operator ==(const WpaSupplicantNetwork &other)
{
    return dbus->path() == other.dbus->path();
}

WpaSupplicantNetwork::~WpaSupplicantNetwork()
{
    delete dbus;
}
