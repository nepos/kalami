/***
  Copyright (c) 2016 Nepos GmbH

  Authors: Daniel Mack <daniel@nepos.io>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
***/

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
    QJsonArray jsonArray;

    QList<QDBusObjectPath> list = interface->bSSs();

    for (int i = 0; i < list.size(); i++) {
        FiW1Wpa_supplicant1BSSInterface bss("fi.w1.wpa_supplicant1", list.at(i).path(),
                                            QDBusConnection::systemBus(), this);

        if (!bss.isValid())
            continue;

        QJsonObject jsonObject {
            { "ssid", qPrintable(bss.sSID()) },
            { "bssid", qPrintable(bss.bSSID().toHex()) },
            { "signal", bss.signal() },
            { "privacy", bss.privacy() },
        };

        jsonArray << jsonObject;
    }

    emit networkScanCompleted(jsonArray);
}

WifiManager::~WifiManager()
{
    delete wpaSupplicant;
}
