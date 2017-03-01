/***
  Copyright (c) 2017 Nepos GmbH

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

#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusObjectPath>
#include <QDBusMetaType>
#include <QDebug>

#include "daemon.h"
#include "types.h"

Daemon::Daemon(QUrl uri, QObject *parent) :
    QObject(parent)
{
    // Redux/websocket connection
    redux = new ReduxProxy(uri);
    QObject::connect(redux, &ReduxProxy::stateUpdated, this, [this](const QJsonObject &state) {
        qDebug() << "STATE:" << state;

        if (state.contains("Network")) {
            QJsonObject network = state["Network"].toObject();

            if (network.contains("knownWifis")) {
                QJsonArray knownWifis = network["knownWifis"].toArray();
                connman->updateKnownWifis(knownWifis);
            }
        }
    });

    // D-Bus connection
    QDBusConnection bus = QDBusConnection::systemBus();
    if (bus.isConnected())
        qInfo() << "Connected to D-Bus as" << bus.baseService();
    else
        qWarning() << "D-Bus connection failed:" << bus.lastError();

    systemdConnection = new QDBusInterface("org.freedesktop.systemd1",
                                           "/org/freedesktop/systemd1",
                                           "org.freedesktop.systemd1.Manager",
                                           bus, this);

    // Connman connection
    connman = new Connman();
    QObject::connect(connman, &Connman::availableWifisUpdated, this, [this](const QJsonArray &list) {
        QJsonObject action {
            { "type",           "NETWORK:UPDATE_AVAILABLE_WIFIS" },
            { "availableWifis", list },
        };

        redux->dispatchAction(action);
    });

    // udev monitor
    udev = new UDevMonitor();

    QObject::connect(udev, &UDevMonitor::deviceAdded, this, [this](const UDevDevice &d) {
        qDebug() << "Linux device added:" << d.getDevPath() << "sysname" << d.getSysName();
    });

    QObject::connect(udev, &UDevMonitor::deviceRemoved, this, [this](const UDevDevice &d) {
        qDebug() << "Linux device removed:" << d.getDevPath() << "sysname" << d.getSysName();
    });

    udev->addMatchSubsystem("input");
}

Daemon::~Daemon()
{
    delete systemdConnection;
    delete connman;
    delete udev;
    delete redux;
}
