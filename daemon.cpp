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
    volumeInputDevice(new InputDevice("/sys/devices/platform/rotary/input/input1")),
    connman(new Connman(this)),
    machine(new Machine(this)),
    fring(new Fring()),
    polyphant(new PolyphantConnection(uri, this)),
    updater(new Updater(machine, "latest", this)),
    nfc(new Nfc(this))
{
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

    QObject::connect(updater, &Updater::updateProgress, this, [this](float progress) {
        qInfo(DaemonLog) << "Updater progress:" << progress;
    });

    // ALSA
    qInfo(DaemonLog) << "Current master volume:" << mixer->getMasterVolume();

    // Input devices
    QObject::connect(volumeInputDevice, &InputDevice::inputEvent, this, [this](int type, int code, int value) {
        if (type != EV_REL || code != REL_X)
            return;

        PolyphantMessage msg(value > 0 ?
                                 "volumecontrol/VOLUME_UP" :
                                 "volumecontrol/VOLUME_DOWN",
                             QJsonObject{}, 0);
        polyphant->sendMessage(msg);
    });

    // Connman connection
    QObject::connect(connman, &Connman::availableWifisUpdated, this, [this](const QJsonArray &list) {
        PolyphantMessage msg("wifi/SCAN_RESULT", list, 0);
        polyphant->sendMessage(msg);
    });

    QObject::connect(connman, &Connman::connectedWifiChanged, this, [this](const QJsonObject &wifi) {
        // TODO
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

    // fring
    if (fring->initialize()) {
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

        QObject::connect(fring, &Fring::logMessageReceived, this, [this](const QString &message) {
            qInfo(DaemonLog) << "Message from fring:" << message;
        });
    }
}

void Daemon::polyphantMessageReceived(const PolyphantMessage &message)
{
    bool ret = true;
    QJsonObject payload = message.payload().toObject();

    qInfo() << "MSG PL:" << payload;
    qInfo() << "value:" << payload["value"] << payload["value"].toDouble();

    if (message.type() == "display/SET_BRIGHTNESS") {
        displayBrightness->setBrightness(payload["value"].toDouble());
    }

    if (message.type() == "led/SET_STATE") {
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

    if (message.type() == "wifi/CONNECT") {
    }

    if (message.type() == "wifi/DISCONNECT") {

    }

    Q_UNUSED(ret);
}

Daemon::~Daemon()
{
}
