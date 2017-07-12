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

Q_LOGGING_CATEGORY(DaemonLog, "Daemon")

Daemon::Daemon(QUrl uri, QObject *parent) :
    QObject(parent),
    mixer(new ALSAMixer("hw:0", this)),
    lightSensor(new AmbientLightSensor("/sys/bus/iio/devices/iio:device0/in_illuminance0_input")),
    displayBrightness(new BrightnessControl("/sys/class/backlight/1a98000.dsi.0")),
    updater(new Updater(machine, "latest", this)),
    connman(new Connman(this)),
    machine(new Machine(this)),
    systemdConnection(new QDBusInterface("org.freedesktop.systemd1",
                                         "/org/freedesktop/systemd1",
                                         "org.freedesktop.systemd1.Manager",
                                         QDBusConnection::systemBus(), this)),
    fring(new Fring()),
    polyphant(new PolyphantConnection(uri, this)),
    udev(new UDevMonitor(this)),
    nfc(new Nfc(this))
{
    // ALSA
    qInfo(DaemonLog) << "Current master volume:" << mixer->getMasterVolume();

    // Updater logic
    QObject::connect(updater, &Updater::updateAvailable, this, [this](const QString &version) {
        qInfo(DaemonLog) << "New update available, version" << version;
        updater->install();
    });

    QObject::connect(updater, &Updater::alreadyUpToDate, this, [this]() {
        qInfo(DaemonLog) << "Already up-to-date!";
    });

    QObject::connect(updater, &Updater::checkFailed, this, [this]() {
        qInfo(DaemonLog) << "Update check failed!";
    });

    QObject::connect(updater, &Updater::updateSucceeded, this, [this]() {
        qInfo(DaemonLog) << "Update succeeded!";
    });

    QObject::connect(updater, &Updater::updateFailed, this, [this]() {
        qInfo(DaemonLog) << "Update failed!";
    });

    // Connman connection
    QObject::connect(connman, &Connman::availableWifisUpdated, this, [this](const QJsonArray &list) {
        QJsonObject action {
            { "type",           "NETWORK:UPDATE_AVAILABLE_WIFIS" },
            { "availableWifis", list },
        };

        //redux->dispatchAction(action);
    });

    QObject::connect(connman, &Connman::connectedWifiChanged, this, [this](const QJsonObject &wifi) {
        QJsonObject action {
            { "type", "NETWORK:CONNECTED_WIFI_CHANGED" },
            { "wifi", wifi },
        };

        //redux->dispatchAction(action);
    });

    QObject::connect(connman, &Connman::goneOnline, this, [this]() {
        qInfo(DaemonLog) << "We are now online!";
        updater->check();
    });

    connman->start();

    // Websocket connection
    QObject::connect(polyphant, &PolyphantConnection::messageReceived, this, &Daemon::polyphantMessageReceived);

    // light sensor
    QObject::connect(lightSensor, &AmbientLightSensor::valueChanged, this, [this](float value) {
        PolyphantMessage msg("ambientlight/STATE_CHANGED", QJsonObject {
                                 { "id", "primary" },
                                 { "value", value },
                             }, 0);
        polyphant->sendMessage(msg);
    });

    lightSensor->start();

    // D-Bus connection
    QDBusConnection bus = QDBusConnection::systemBus();
    if (bus.isConnected())
        qInfo(DaemonLog) << "Connected to D-Bus as" << bus.baseService();
    else
        qWarning(DaemonLog) << "D-Bus connection failed:" << bus.lastError();


    // udev monitor
    udev->addMatchSubsystem("input");

    QObject::connect(udev, &UDevMonitor::deviceAdded, this, [this](const UDevDevice &d) {
        qInfo(DaemonLog) << "Linux device added:" << d.getDevPath() << "sysname" << d.getSysName();
    });

    QObject::connect(udev, &UDevMonitor::deviceRemoved, this, [this](const UDevDevice &d) {
        qInfo(DaemonLog) << "Linux device removed:" << d.getDevPath() << "sysname" << d.getSysName();
    });

    // fring
    QObject::connect(fring, &Fring::homeButtonChanged, this, [this](bool state) {
        PolyphantMessage msg("homebutton/STATE_CHANGED", QJsonObject {
                                 { "id", "home" },
                                 { "state", state },
                             }, 0);
        polyphant->sendMessage(msg);
    });

    QObject::connect(fring, &Fring::batteryStateChanged, this, [this](float level, float chargeCurrent, float dischargeCurrent) {
        PolyphantMessage msg("battery/STATE_CHANGED", QJsonObject {
                                 { "level", level },
                                 { "chargeCurrent", chargeCurrent },
                                 { "dischargeCurrent", dischargeCurrent },
                             }, 0);
        polyphant->sendMessage(msg);
    });
}

void Daemon::polyphantMessageReceived(const PolyphantMessage &message)
{
    bool ret = true;

    if (message.type() == "display/SET_BRIGHTNESS") {
        QJsonObject payload = message.payload();
        displayBrightness->setBrightness(payload["value"].toDouble());
    }

    if (message.type() == "led/SET_STATE") {
        QJsonObject payload = message.payload();
        QJsonObject color = payload["color"].toObject();
        int id = payload["id"] == "videocall" ? 0 : 1;

        if (payload["mode"] == "off")
            ret = fring->setLedOff(id);

        if (payload["mode"] == "on")
            ret = fring->setLedOn(id, color["red"].toDouble(), color["green"].toDouble(), color["blue"].toDouble());

        if (payload["mode"] == "blink")
            ret = fring->setLedFlashing(id, color["red"].toDouble(), color["green"].toDouble(), color["blue"].toDouble(),
                                        payload["onPhase"].toDouble(), payload["offPhase"].toDouble());

        if (payload["mode"] == "pulse")
            ret = fring->setLedPulsating(id, color["red"].toDouble(), color["green"].toDouble(), color["blue"].toDouble(),
                                         payload["frequency"].toDouble());
    }

    Q_UNUSED(ret);
}

Daemon::~Daemon()
{
}
