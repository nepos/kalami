#include <QDebug>

#include "wifimanager.h"

WifiManager::WifiManager(QObject *parent) :
    QObject(parent), statusString(), interfaces()
{
    wpaSupplicant = new FiW1Wpa_supplicant1Interface("fi.w1.wpa_supplicant1", "/fi/w1/wpa_supplicant1",
                                                     QDBusConnection::systemBus(), this);

    QList<QDBusObjectPath> list = wpaSupplicant->interfaces();

    for (int i = 0; i < list.size(); i++) {
        FiW1Wpa_supplicant1InterfaceInterface *interface =
                new FiW1Wpa_supplicant1InterfaceInterface("fi.w1.wpa_supplicant1", list.at(i).path(),
                                                          QDBusConnection::systemBus(), this);
        monitorInterface(interface);
        interfaces << interface;
    }

    QObject::connect(wpaSupplicant, &FiW1Wpa_supplicant1Interface::InterfaceAdded, this, [this](const QDBusObjectPath &path) {
        FiW1Wpa_supplicant1InterfaceInterface *interface =
                new FiW1Wpa_supplicant1InterfaceInterface("fi.w1.wpa_supplicant1", path.path(),
                                                          QDBusConnection::systemBus(), this);
        monitorInterface(interface);
        interfaces << interface;
        scan();
    });

    QObject::connect(wpaSupplicant, &FiW1Wpa_supplicant1Interface::InterfaceRemoved, this, [this](const QDBusObjectPath &path) {
        for (int i = 0; i < interfaces.size(); i++) {
            if (interfaces[i]->path() == path.path()) {
                delete interfaces[i];
                interfaces.removeAt(i);
                break;
            }
        }
    });

    scan();
}

void WifiManager::monitorInterface(FiW1Wpa_supplicant1InterfaceInterface *interface)
{
    QObject::connect(interface, &FiW1Wpa_supplicant1InterfaceInterface::BSSAdded, this, [this]() {
        scan();
    });

    QObject::connect(interface, &FiW1Wpa_supplicant1InterfaceInterface::BSSRemoved, this, [this]() {
        scan();
    });

    QObject::connect(interface, &FiW1Wpa_supplicant1InterfaceInterface::PropertiesChanged, this, [this, interface]() {
        QString s = interface->state();
        if (s != statusString) {
            qInfo() << "Changing state from" << statusString << "->" << s;
            statusString = s;
        }
    });
}

void WifiManager::scan()
{
    if (interfaces.size() < 1)
        return;

    FiW1Wpa_supplicant1InterfaceInterface *interface = interfaces[0];

    QList<QDBusObjectPath> list = interface->bSSs();

    for (int i = 0; i < list.size(); i++) {
        FiW1Wpa_supplicant1BSSInterface bss("fi.w1.wpa_supplicant1", list.at(i).path(),
                                            QDBusConnection::systemBus(), this);
        if (!bss.isValid())
            continue;

        qInfo() << "SSID" << bss.sSID() << "signal" << bss.signal();
    }
}

WifiManager::~WifiManager()
{
    delete wpaSupplicant;
}
