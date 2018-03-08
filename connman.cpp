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

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

Q_LOGGING_CATEGORY(ConnmanLog, "Connman")

struct ConnmanPrivate {
    ConnmanPrivate() {};
    NetworkManager *manager;
    UserAgent *agent;
    QJsonArray availableWifis;
    QString cachedPassphrase;
    QString cachedWifiId;
    QString cachedWifiState;

    QJsonObject testWifis;
    int testIndex;
    QTimer *testTimer;
    QNetworkReply *testReply;
    QNetworkAccessManager *testNetworkAccessManager;
    QCryptographicHash *testHash;
};

void Connman::connectToKnownWifi()
{
    Q_D(Connman);

    if (d->manager->state() == "online")
        return;

    foreach (NetworkService *service, d->manager->getServices()) {
        if (service->type() != "wifi")
            continue;
    }
}

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

    connectToKnownWifi();

    foreach (const NetworkService *service, d->manager->getServices("wifi")) {
        QString id = kalamiIdForService(service);

        QJsonObject wifi {
            { "kalamiId", id, },
            { "ssid",     service->name() },
            { "security", service->security().join(" ") },
            { "strength", service->strength() / 100.0 },
        };

        //qInfo(ConnmanLog) << "Wifi" << service->name() << "Strength" << service->strength() << "State" << service->state();

        if (id == d->cachedWifiId && service->state() != d->cachedWifiState) {
            QJsonObject wifi {
                { "kalamiId", kalamiIdForService(service) },
                { "connected", service->state() == "ready" || service->state() == "online" },
                { "online", service->state() == "online" },
                { "captivePortalUrl", "" },
            };

            emit wifiChanged(wifi, service->state());

            d->cachedWifiState = service->state();
        }

        d->availableWifis.append(wifi);
    }

    emit availableWifisUpdated(d->availableWifis);
}

void Connman::agentPassphraseRequested(const QString &servicePath, const QVariantMap &fields)
{
    Q_UNUSED(servicePath);
    Q_UNUSED(fields);
    Q_D(Connman);

    QVariantMap reply;
    reply.insert("Passphrase", d->cachedPassphrase);
    d->agent->sendUserReply(reply);
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

void Connman::testDownload()
{
    Q_D(Connman);

    if (d->testReply) {
        d->testReply->abort();
        d->testReply->deleteLater();
    }

    qInfo() << "Attempting download";

    QNetworkRequest req(QUrl("http://zonque.de/test.bin"));
    d->testReply = d->testNetworkAccessManager->get(req);
    d->testReply->setReadBufferSize(1024 * 1024);

    d->testHash->reset();

    QObject::connect(d->testReply, &QNetworkReply::readyRead, [this]() {
        Q_D(Connman);

        if (d->testReply->error() != QNetworkReply::NoError) {
            qInfo(ConnmanLog) << "Error downloading file: " << d->testReply->error();
            d->testReply->abort();
            d->testReply = NULL;
            return;
        }

        const QByteArray data = d->testReply->readAll();
        d->testHash->addData(data);
    });

    connect(d->testReply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
          [this](QNetworkReply::NetworkError code){
        Q_D(Connman);

        qInfo(ConnmanLog) << "Error downloading" << d->testReply->url() << ":" << d->testReply->errorString();
        d->testReply->abort();
        d->testReply = NULL;
        nextTest();
    });

    QObject::connect(d->testReply, &QNetworkReply::finished, [this]() {
        Q_D(Connman);

        if (d->testHash->result().toHex() == "b6699178c2d3f15240ad26804455077a46fce01074605857d083d9f69da87a4ee70a49d515a89b4a4eb44318c465fe4126c4fb98eaf89d45a12c914ce9eeb851") {
            QStringList keys = d->testWifis.keys();
            QString ssid = keys[d->testIndex];

            qInfo(ConnmanLog) << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX SSID" << ssid << "OKAY!";
        } else {
            qInfo(ConnmanLog) << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX WRONG SHA512!?" << d->testHash->result().toHex();
        }

        d->testReply = NULL;

        nextTest();
    });
}

void Connman::checkState()
{
    Q_D(Connman);

    qInfo(ConnmanLog) << "Manager state" << d->manager->state();

    if (d->manager->state() == "offline") {
        d->manager->setOfflineMode(false);
    } else if (d->manager->state() ==  "idle" || d->manager->state() == "ready") {
        //enableWifi();
    } else if (d->manager->state() == "online") {
        //emit goneOnline();

        testDownload();
    }
}

bool Connman::testConnect(int index)
{
    Q_D(Connman);

    QStringList keys = d->testWifis.keys();
    QString ssid = keys[index];
    QString passphrase = d->testWifis[ssid].toString();

    qInfo(ConnmanLog) << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX CONNECTING TO" << ssid << "PW" << passphrase;
    d->cachedPassphrase = passphrase;

    //d->manager->disconnectServices();

    d->testTimer->stop();
    d->testTimer->start(30000);

    foreach (NetworkService *service, d->manager->getServices("wifi")) {
        if (service->name() == ssid) {
            service->requestConnect();
            return true;
        }
    }

    return false;
}

bool Connman::nextTest()
{
    Q_D(Connman);

    d->testIndex++;
    d->testIndex %= d->testWifis.keys().length();

//    d->manager->setOfflineMode(true);
//    QThread::sleep(2);
//    d->manager->setOfflineMode(false);
//    enableWifi();

    return testConnect(d->testIndex);
}

Connman::Connman(QObject *parent) : QObject(parent), d_ptr(new ConnmanPrivate)
{
    Q_D(Connman);

    d->manager = new NetworkManager(this);
    d->availableWifis = QJsonArray();
    d->cachedWifiId = QString();
    d->cachedPassphrase = QString();
    d->cachedWifiState = QString();

    d->testIndex = 0;
    d->testWifis = QJsonObject();
    d->testTimer = new QTimer(this);
    d->testNetworkAccessManager = new QNetworkAccessManager(this);
    d->testReply = NULL;
    d->testHash = new QCryptographicHash(QCryptographicHash::Sha512);

    QObject::connect(d->manager, &NetworkManager::stateChanged, [this, d]() {
        checkState();
    });

    QObject::connect(d->manager, &NetworkManager::technologiesChanged, [this, d]() {
        enableWifi();
    });

    QObject::connect(d->manager, &NetworkManager::offlineModeChanged, [this, d]() {
        qInfo(ConnmanLog) << "connman: offlineModeChanged" << d->manager->offlineMode();

        if (!d->manager->offlineMode())
            enableWifi();

        //iterateServices();
    });

    QObject::connect(d->manager, &NetworkManager::servicesChanged, [this]() {
        //iterateServices();
    });

    d->agent = new UserAgent(this);
    d->agent->setAgentPath("/io/nepos/ConnmanAgent");
    QObject::connect(d->agent, &UserAgent::userInputRequested, this, &Connman::agentPassphraseRequested);


    d->testTimer->setSingleShot(true);
    QObject::connect(d->testTimer, &QTimer::timeout, [this, d]() {
        QStringList keys = d->testWifis.keys();
        QString ssid = keys[d->testIndex];
        qInfo(ConnmanLog) << "TIMEOUT waiting for Wifi" << ssid << "to become ready.";
        nextTest();
    });

    QFile file("/var/wifis.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        d->testWifis = QJsonDocument::fromJson(data).object();
        d->testIndex = 0;
        nextTest();
    }
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

    return false;

    d->cachedPassphrase = passphrase;
    d->cachedWifiId = wifiId;
    d->cachedWifiState.clear();

    foreach (NetworkService *service, d->manager->getServices("wifi")) {
        QString id = kalamiIdForService(service);

        if (id == wifiId) {
            service->requestConnect();
            return true;
        }
    }

    return false;
}

bool Connman::disconnectFromWifi(const QString &wifiId)
{
    Q_D(Connman);

    return false;

    foreach (NetworkService *service, d->manager->getServices("wifi")) {
        QString id = kalamiIdForService(service);

        if (id == wifiId) {
            service->disconnect();
            return true;
        }
    }

    return false;
}
