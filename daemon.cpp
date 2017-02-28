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

#include "connman.h"
#include "daemon.h"
#include "types.h"

Daemon::Daemon(QUrl uri, QObject *parent) :
    QObject(parent)
{
    // Redux/websocket connection
    redux = new ReduxProxy(uri);
    QObject::connect(redux, &ReduxProxy::stateUpdated, this, [this](const QJsonObject &state) {
        qDebug() << "STATE:" << state;
    });

    // D-Bus connection
    QDBusConnection bus = QDBusConnection::systemBus();
    if (bus.isConnected())
        qInfo() << "Connected to D-Bus as" << bus.baseService();
    else
        qWarning() << "D-Bus connection failed:" << bus.lastError();

    // QNetworkConfigurationManager
    networkManager = new QNetworkConfigurationManager(this);
    QList<QNetworkConfiguration> configs = networkManager->allConfigurations(QNetworkConfiguration::Undefined);

    for (int i = 0; i < configs.size(); i++) {
        QNetworkConfiguration config = configs[i];
        qDebug() << "EXISTING:" << config.name() << " - " << config.identifier() << "valid? " << config.isValid();
    }

    qDebug() << "online?" << networkManager->isOnline();

    QObject::connect(networkManager, &QNetworkConfigurationManager::configurationAdded, this, [this](const QNetworkConfiguration &config) {
        qDebug() << "NEW:" << config.name() << " - " << config.identifier() << "valid? " << config.isValid();
    });

    QObject::connect(networkManager, &QNetworkConfigurationManager::configurationChanged, this, [this](const QNetworkConfiguration &config) {
        qDebug() << "CHANGED:" << config.name() << " - " << config.identifier() << "valid? " << config.isValid();
    });

    QObject::connect(networkManager, &QNetworkConfigurationManager::configurationRemoved, this, [this](const QNetworkConfiguration &config) {
        qDebug() << "REMOVED:" << config.name() << " - " << config.identifier() << "valid? " << config.isValid();
    });

    Connman *c = new Connman();

    systemdConnection = new QDBusInterface("org.freedesktop.systemd1",
                                           "/org/freedesktop/systemd1",
                                           "org.freedesktop.systemd1.Manager",
                                           bus, this);

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
    delete networkManager;
    delete udev;
}
