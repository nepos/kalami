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

#include <qconnman/agent.h>
#include <qconnman/manager.h>
#include <qconnman/technology.h>

Q_LOGGING_CATEGORY(ConnmanLog, "Connman")

struct ConnmanPrivate {
    ConnmanPrivate() {};
    Manager *manager;
    Agent *agent;
    QJsonArray availableWifis;
    QString cachedPassphrase;
    QString cachedWifiId;
};

void Connman::connectToKnownWifi()
{
    Q_D(Connman);

    if (d->manager->state() == Manager::Online)
        return;

    foreach (Service *service, d->manager->services()) {
        if (service->type() != "wifi")
            continue;
    }
}

void Connman::iterateServices()
{
    Q_D(Connman);

    d->availableWifis = QJsonArray();

    connectToKnownWifi();

    foreach (const Service *service, d->manager->services()) {
        if (service->type() != "wifi")
            continue;

        QString state;

        switch (service->state()) {
        case Service::UndefinedState:
            state = "undefined";
            break;
        case Service::IdleState:
            state = "idle";
            break;
        case Service::FailureState:
            state = "failed";
            break;
        case Service::AssociationState:
            state = "associated";
            break;
        case Service::ConfigurationState:
            state = "configuring";
            break;
        case Service::ReadyState:
            state = "ready";
            break;
        case Service::DisconnectState:
            state = "disconnecting";
            break;
        case Service::OnlineState:
            state = "online";
            break;
        }

        QString id = service->name().toUtf8().toBase64();

        QJsonObject wifi {
            { "kalamiId", id, },
            { "ssid",     service->name() },
            { "security", service->security().join(" ") },
            { "strength", service->strength() / 100.0 },
            { "state",    state },
        };

        //qInfo(ConnmanLog) << "Wifi" << service->name() << "Strength" << service->strength() << "State" << state;

        d->availableWifis.append(wifi);
    }

    emit availableWifisUpdated(d->availableWifis);
}

void Connman::agentPassphraseRequested()
{
    Q_D(Connman);

    Agent::InputRequest *request = d->agent->currentInputRequest();
    Service *service = d->manager->service(request->service);
    QString id = service->name().toUtf8().toBase64();

    if (id == d->cachedWifiId) {
        request->response.name = service->name();
        request->response.passphrase = d->cachedPassphrase;
    } else {
        request->cancel = true;
    }
}

void Connman::sendConnectedService()
{
    Q_D(Connman);

    Service *service = d->manager->connectedService();
    if (!service)
        return;

    QJsonObject wifi {
        { "SSID", service->name() },
    };

    emit connectedWifiChanged(wifi);
}

void Connman::enableWifi()
{
    Q_D(Connman);

    foreach (Technology *technology, d->manager->technologies()) {
        qInfo(ConnmanLog) << "Technology:" << technology->name();
        if (technology->name() == "WiFi") {
            qInfo(ConnmanLog) << "Enabling Wifi";
            technology->setPowered(true);
            technology->scan();
        }
    }
}

void Connman::checkState()
{
    Q_D(Connman);

    qInfo(ConnmanLog) << "Manager state" << d->manager->state();

    switch (d->manager->state()) {
    case Manager::Offline:
        d->manager->setOfflineMode(false);
        break;
    case Manager::Idle:
    case Manager::Ready:
        enableWifi();
        break;
    case Manager::Online:
        emit goneOnline();
        break;
    default:
        break;
    }
}

Connman::Connman(QObject *parent) : QObject(parent), d_ptr(new ConnmanPrivate)
{
    Q_D(Connman);

    d->manager = new Manager(this);
    d->availableWifis = QJsonArray();

    QObject::connect(d->manager, &Manager::stateChanged, [this, d]() {
        checkState();
    });

    QObject::connect(d->manager, &Manager::offlineModeChanged, [this, d]() {
        qInfo(ConnmanLog) << "connman: offlineModeChanged" << d->manager->offlineMode();

        if (!d->manager->offlineMode())
            enableWifi();

        iterateServices();
    });

    QObject::connect(d->manager, &Manager::connectedServiceChanged, [this]() {
        qInfo(ConnmanLog) << "connman: connectedServiceChanged";
        sendConnectedService();
    });

    // Must use the deprecated SIGNAL/SLOT macros as Manager::serviceChanged is overloaded
    QObject::connect(d->manager, SIGNAL(servicesChanged()), this, SLOT(iterateServices()));

    d->agent = new Agent("/io/nepos/ConnmanAgent", d->manager);
    QObject::connect(d->agent, &Agent::passphraseRequested, this, &Connman::agentPassphraseRequested);
}

void Connman::start()
{
    Q_D(Connman);

    d->manager->setOfflineMode(false);
    enableWifi();
    checkState();
}

bool Connman::connectToWifi(const QString &wifiId, const QString &passphrase)
{
    Q_D(Connman);

    d->cachedPassphrase = passphrase;
    d->cachedWifiId = wifiId;

    foreach (Service *service, d->manager->services()) {
        if (service->type() != "wifi")
            continue;

        QString id = service->name().toUtf8().toBase64();

        if (id == wifiId) {
            service->connect();
            return true;
        }
    }

    return false;
}

bool Connman::disconnectFromWifi(const QString &wifiId)
{
    Q_D(Connman);

    foreach (Service *service, d->manager->services()) {
        if (service->type() != "wifi")
            continue;

        QString id = service->name().toUtf8().toBase64();

        if (id == wifiId) {
            service->disconnect();
            return true;
        }
    }

    return false;
}
