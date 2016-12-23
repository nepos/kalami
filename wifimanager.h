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

#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <QObject>

#include "types.h"
#include "dbus_fi.w1.wpa_supplicant1.h"
#include "dbus_fi.w1.wpa_supplicant1.Interface.h"
#include "dbus_fi.w1.wpa_supplicant1.Network.h"
#include "dbus_fi.w1.wpa_supplicant1.BSS.h"

class WifiManager : public QObject
{
    Q_OBJECT
public:
    explicit WifiManager(QObject *parent = 0);
    ~WifiManager();

signals:

public slots:

private:
    QString statusString;
    FiW1Wpa_supplicant1Interface *wpaSupplicant;
    QList<FiW1Wpa_supplicant1InterfaceInterface*> interfaces;

    void scan();
    void monitorInterface(FiW1Wpa_supplicant1InterfaceInterface *interface);
};

#endif // WIFIMANAGER_H
