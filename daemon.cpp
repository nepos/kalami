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
#include <QJsonObject>
#include <QByteArray>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusObjectPath>
#include <QDBusMetaType>
#include <QDebug>

#include "connmanagent.h"
#include "daemon.h"
#include "types.h"

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

    QObject::connect(socket, &QWebSocket::textMessageReceived, [this](const QString &message) {
        QJsonDocument doc = QJsonDocument::fromJson(message.toLocal8Bit());

        if (doc.isObject()) {
            QJsonObject obj = doc.object();

            dispatchSocketMessage(obj.value("type"),
                                  obj.value("element"),
                                  obj.value("value"));
        }
    });

    socket->open(serverUri);

    // D-Bus connection
    QDBusConnection bus = QDBusConnection::systemBus();
    if (bus.isConnected())
        qInfo() << "Connected to D-Bus as" << bus.baseService();
    else
        qWarning() << "D-Bus connection failed: " << bus.lastError();

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

    ConnmanAgent *c = new ConnmanAgent();
    bool success = QDBusConnection::systemBus().registerObject("/io/nepos/connman/Agent", c);
    qDebug() << "success?" << success;

    systemdConnection = new QDBusInterface("org.freedesktop.systemd1",
                                           "/org/freedesktop/systemd1",
                                           "org.freedesktop.systemd1.Manager",
                                           bus, this);

    // udev monitor
    udev = new UDevMonitor();

    QObject::connect(udev, &UDevMonitor::deviceAdded, this, [this](const UDevDevice &d) {
        //qDebug() << "DEVICE ADDED: " << d.getDevPath() << "sysname" << d.getSysName();
    });

    QObject::connect(udev, &UDevMonitor::deviceRemoved, this, [this](const UDevDevice &d) {
        qDebug() << "DEVICE REMOVED: " << d.getDevPath() << "sysname" << d.getSysName();
    });

    udev->addMatchSubsystem("input");
}

void Daemon::dispatchSocketMessage(const QJsonValue &type, const QJsonValue &element, const QJsonValue &value)
{
    qDebug() << "type" << type << "element" << element << "value" << value;
}

void Daemon::sendSocketMessage(const QString &type, const QString &element, const QString &value)
{
    QJsonObject obj {
        { "type", type },
        { "element", element },
        { "value", value },
    };

    QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    socket->sendBinaryMessage(payload);
}

Daemon::~Daemon()
{
    delete socket;
    delete systemdConnection;
    delete networkManager;
    delete udev;
}
