#include <QDebug>

#include "wpasupplicantinterface.h"

WpaSupplicantInterface::WpaSupplicantInterface(const QDBusObjectPath &path, QObject *parent) :
    QObject(parent), networks(), bsss()
{
    dbus = new QDBusInterface("fi.w1.wpa_supplicant1",
                              path.path(),
                              "fi.w1.wpa_supplicant1.Interface",
                              QDBusConnection::systemBus(), this);

    QList<QDBusObjectPath> list = dbus->property("Networks").value<QList<QDBusObjectPath> >();

    for (int i = 0; i < list.size(); i++)
        networks.append(new WpaSupplicantNetwork(list.at(i)));

    dbus->property("BSSs").value<QList<QDBusObjectPath> >();

    for (int i = 0; i < list.size(); i++)
        bsss.append(new WpaSupplicantBss(list.at(i)));
}

void WpaSupplicantInterface::start()
{
}

const QString WpaSupplicantInterface::getIfname() const
{
    return QString(dbus->property("Ifname").toString());
}

bool WpaSupplicantInterface::operator ==(const WpaSupplicantInterface &other)
{
    return dbus->path() == other.dbus->path();
}

WpaSupplicantInterface::~WpaSupplicantInterface()
{
    while (!networks.isEmpty())
        delete networks.takeFirst();

    while (!bsss.isEmpty())
        delete bsss.takeFirst();

    delete dbus;
}
