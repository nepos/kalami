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

#include <QTimer>
#include <QJsonDocument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusError>
#include <QDebug>

#include "daemon.h"

Daemon::Daemon(QUrl uri, QObject *parent) :
    QObject(parent), serverUri(uri)
{
    // WebSocket connection
    socket = new QWebSocket();

    QObject::connect(socket, &QWebSocket::connected, [this]() {
    });

    QObject::connect(socket, &QWebSocket::disconnected, [this]() {
        QTimer::singleShot(1000, [this]() {
            socket->open(serverUri);
        });
    });

    socket->open(serverUri);

    // D-Bus connection
    QDBusConnection bus = QDBusConnection::systemBus();
    if (bus.isConnected())
        qDebug() << "Connected to D-Bus as" << bus.baseService();
    else
        qWarning() << "D-Bus connection failed: " << bus.lastError();

    systemd = new QDBusInterface("org.freedesktop.systemd1",
                                 "/org/freedesktop/systemd1",
                                 "org.freedesktop.systemd1.Manager",
                                 bus, this);

    udev = new UDevMonitor();

    QObject::connect(udev, &UDevMonitor::deviceAdded, this, [this](const UDevDevice &d) {
        qDebug() << "DEVICE ADDED: " << d.getDevPath() << "sysname" << d.getSysName();
    });

    QObject::connect(udev, &UDevMonitor::deviceRemoved, this, [this](const UDevDevice &d) {
        qDebug() << "DEVICE REMOVED: " << d.getDevPath() << "sysname" << d.getSysName();
    });

    udev->addMatchSubsystem("input");
}

Daemon::~Daemon()
{
    delete socket;
    delete systemd;
    delete udev;
}
