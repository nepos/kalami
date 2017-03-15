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
    QJsonArray knownWifis;
    QJsonArray availableWifis;
};

void Connman::connectToKnownWifi()
{
    Q_D(Connman);

    if (d->manager->state() == Manager::Online)
        return;

    foreach (Service *service, d->manager->services()) {
        if (service->type() != "wifi")
            continue;

        foreach (const QJsonValue value, d->knownWifis) {
            const QJsonObject knownWifi = value.toObject();

            if (service->name() == knownWifi["SSID"].toString() &&
                service->state() == Service::IdleState) {
                service->connect();
                qInfo(ConnmanLog) << "Connecting to known wifi" << service->name();
                return;
            }
        }
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

        QJsonObject wifi {
            { "SSID",     service->name() },
            { "security", service->security().join(" ") },
            { "strength", service->strength() / 100.0 },
            { "state",    state },
        };

        //qInfo(ConnmanLog) << "Wifi" << service->name() << "Strength" << service->strength() << "State" << state;

        d->availableWifis.append(wifi);
    }

    emit availableWifisUpdated(d->availableWifis);
}

void Connman::updateKnownWifis(QJsonArray &list)
{
    Q_D(Connman);

    d->knownWifis = list;
    connectToKnownWifi();
}

void Connman::agentPassphraseRequested()
{
    Q_D(Connman);

    Agent::InputRequest *request = d->agent->currentInputRequest();
    Service *service = d->manager->service(request->service);

    foreach (const QJsonValue value, d->knownWifis) {
        const QJsonObject knownWifi = value.toObject();

        if (service->name() == knownWifi["SSID"].toString() && knownWifi.contains("Passphrase")) {
            request->response.name = service->name();
            request->response.passphrase = knownWifi["Passphrase"].toString();
            return;
        }
    }

    // The wifi we're looking for is not in 'knownWifis', so let's augment the 'availableWifis' array.
    foreach (QJsonValue value, d->availableWifis) {
        QJsonObject availableWifi = value.toObject();

        if (service->name() == availableWifi["SSID"].toString()) {
            availableWifi["PassphraseRequired"] = true;
            break;
        }
    }

    request->cancel = true;

    emit availableWifisUpdated(d->availableWifis);
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

Connman::Connman(QObject *parent) : QObject(parent), d_ptr(new ConnmanPrivate)
{
    Q_D(Connman);

    d->manager = new Manager(this);
    d->knownWifis = QJsonArray();
    d->availableWifis = QJsonArray();

    QObject::connect(d->manager, &Manager::stateChanged, [this, d]() {
        qInfo(ConnmanLog) << "connman: stateChanged" << d->manager->state();
        if (d->manager->state() == Manager::Online)
            emit goneOnline();
    });

    QObject::connect(d->manager, &Manager::offlineModeChanged, [this, d]() {
        qInfo(ConnmanLog) << "connman: offlineModeChanged" << d->manager->offlineMode();
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

    foreach (Technology *technology, d->manager->technologies()) {
        qInfo(ConnmanLog) << "tech name" << technology->name() << "type" << technology->type();
        if (technology->name() == "WiFi") {
            technology->setPowered(true);
            technology->scan();
        }
    }

    iterateServices();
    sendConnectedService();

    qInfo() << "Manager state" << d->manager->state();

    if (d->manager->state() == Manager::Online)
        emit goneOnline();
}
