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

#include <QDebug>
#include <QTimer>
#include "connman.h"

#include <connman-qt5/useragent.h>
#include <connman-qt5/networkmanager.h>
#include <connman-qt5/networktechnology.h>

Q_LOGGING_CATEGORY(ConnmanLog, "Connman")

struct ConnmanPrivate {
    ConnmanPrivate() {};
    NetworkManager *manager;
    QString currentWifiId;
    QString currentWifiCachedPassphrase;
    QString currentWifiLastReportedState;
    UserAgent *agent;
    QJsonArray availableWifis;
    QString preconfiguredSSID;
    QString preconfiguredPassword;
};

QString Connman::kalamiIdForService(const NetworkService *service)
{
    QStringList list;
    list << service->name() << service->bssid();
    return list.join("-").toUtf8().toBase64();
}

void Connman::iterateServices()
{
    Q_D(Connman);

    d->availableWifis = QJsonArray();

    foreach (const NetworkService *service, d->manager->getServices("wifi")) {
        QString id = kalamiIdForService(service);

        QJsonObject wifi {
            { "kalamiId", id, },
            { "ssid",     service->name() },
            { "security", service->security().join(" ") },
            { "strength", service->strength() / 100.0 },
        };

        qInfo(ConnmanLog) << "Wifi" << service->name() << "Strength" << service->strength() << "State" << service->state();

        if (id == d->currentWifiId && service->state() != d->currentWifiLastReportedState) {
            bool online = service->state() == "online";
            bool connected = online || service->state() == "ready";

            QJsonObject wifi {
                { "kalamiId", kalamiIdForService(service) },
                { "connected", connected },
                { "online", online },
                { "captivePortalUrl", "" },
            };

            if (online) {
                QVariantMap ipv4 = service->ipv4();
                QVariantMap ipv6 = service->ipv6();

                wifi["ipv4Address"] = ipv4["Address"].toString();
                wifi["ipv6Address"] = ipv6["Address"].toString();
            }

            emit wifiChanged(wifi, service->state());

            d->currentWifiLastReportedState = service->state();
        }

        // If there is a preconfigured SSID, try to connect to it once
        if (!d->preconfiguredSSID.isEmpty() && d->preconfiguredSSID == service->name()) {
            qInfo(ConnmanLog) << "Connecting to preconfigured SSID" << service->name();
            connectToWifi(id, QString(d->preconfiguredPassword), false);
            d->preconfiguredSSID.clear();
            d->preconfiguredPassword.clear();
        }

        d->availableWifis.append(wifi);
    }

    emit availableWifisUpdated(d->availableWifis);
}

void Connman::enableWifi()
{
    Q_D(Connman);

    NetworkTechnology *technology = d->manager->getTechnology("wifi");
    if (technology && !technology->powered()) {
        technology->setPowered(true);
        technology->scan();
    }
}

Connman::Connman(QObject *parent) : QObject(parent), d_ptr(new ConnmanPrivate)
{
    Q_D(Connman);

    d->manager = new NetworkManager(this);
    d->currentWifiId = QString();
    d->availableWifis = QJsonArray();
    d->currentWifiCachedPassphrase = QString();
    d->currentWifiLastReportedState = QString();
    d->preconfiguredSSID = QString::fromLocal8Bit(qgetenv("WLAN_SSID"));
    d->preconfiguredPassword = QString::fromLocal8Bit(qgetenv("WLAN_PW"));

    QObject::connect(d->manager, &NetworkManager::stateChanged, [this, d]() {
        qInfo(ConnmanLog) << "Manager state" << d->manager->state();

        if (d->manager->state() == "offline") {
            d->manager->setOfflineMode(false);
        } else if (d->manager->state() ==  "idle" || d->manager->state() == "ready") {
            enableWifi();
        }
    });

    QObject::connect(d->manager, &NetworkManager::technologiesChanged, [this, d]() {
        enableWifi();
    });

    QObject::connect(d->manager, &NetworkManager::offlineModeChanged, [this, d]() {
        qInfo(ConnmanLog) << "connman: offlineModeChanged" << d->manager->offlineMode();

        if (!d->manager->offlineMode())
            enableWifi();

        iterateServices();
    });

    QObject::connect(d->manager, &NetworkManager::servicesChanged, [this]() {
        iterateServices();
    });

    d->agent = new UserAgent(this);
    d->agent->setAgentPath("/io/nepos/ConnmanAgent");

    QObject::connect(d->agent, &UserAgent::userInputRequested, [this, d](const QString &servicePath, const QVariantMap &fields) {
        Q_UNUSED(servicePath);
        Q_UNUSED(fields);

        QVariantMap reply;
        reply.insert("Passphrase", d->currentWifiCachedPassphrase);
        d->agent->sendUserReply(reply);
    });
}

void Connman::start()
{
    Q_D(Connman);

    d->manager->setOfflineMode(false);
    enableWifi();
}

bool Connman::connectToWifi(const QString &wifiId, const QString &passphrase, bool iterateImmediately)
{
    Q_D(Connman);

    foreach (NetworkService *service, d->manager->getServices("wifi")) {
        QString id = kalamiIdForService(service);

        if (id == wifiId) {
            d->currentWifiId = id;
            d->currentWifiCachedPassphrase = passphrase;
            d->currentWifiLastReportedState.clear();

            service->setAutoConnect(false);
            service->requestConnect();

            if (iterateImmediately)
                iterateServices();

            return true;
        }
    }

    return false;
}

bool Connman::disconnectFromWifiById(const QString &wifiId)
{
    Q_D(Connman);

    foreach (NetworkService *service, d->manager->getServices("wifi")) {
        QString id = kalamiIdForService(service);

        if (id == wifiId) {
            service->setAutoConnect(false);
            service->disconnect();
            return true;
        }
    }

    return false;
}

bool Connman::disconnectFromWifi(const QString &wifiId)
{
    Q_D(Connman);

    d->currentWifiId.clear();

    return disconnectFromWifiById(wifiId);
}

void Connman::suspend()
{
    Q_D(Connman);

    if (!d->currentWifiId.isEmpty())
        disconnectFromWifiById(d->currentWifiId);
}

void Connman::resume()
{
    Q_D(Connman);

    enableWifi();

    if (d->currentWifiId.isEmpty())
        return;

    foreach (NetworkService *service, d->manager->getServices("wifi")) {
        QString id = kalamiIdForService(service);

        if (id == d->currentWifiId) {
            d->currentWifiLastReportedState.clear();

            service->setAutoConnect(false);
            service->requestConnect();
            iterateServices();
            return;
        }
    }
}

