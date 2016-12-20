#include <QDebug>
#include "wpasupplicant.h"

WpaSupplicant::WpaSupplicant(QObject *parent) : QObject(parent)
{
    dbus = new QDBusInterface("fi.w1.wpa_supplicant1",
                              "/fi/w1/wpa_supplicant1",
                              "fi.w1.wpa_supplicant1",
                              QDBusConnection::systemBus(), this);
}

void WpaSupplicant::start()
{
    QList<QDBusObjectPath> list = dbus->property("Interfaces").value<QList<QDBusObjectPath> >();

    for (int i = 0; i < list.size(); i++) {
        WpaSupplicantInterface *interface = new WpaSupplicantInterface(list.at(i));
        interfaces.append(interface);
        emit interfaceAdded(*interface);
     }
}

WpaSupplicant::~WpaSupplicant()
{
    while (!interfaces.isEmpty())
        delete interfaces.takeFirst();

    delete dbus;
}
