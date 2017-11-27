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
    accelerometer(new Accelerometer("/dev/input/by-path/platform-lis3lv02d-event", this)),
    mixer(new ALSAMixer("hw:0", this)),
    displayBrightness(new BrightnessControl("/sys/class/backlight/1a98000.dsi.0")),
    rotaryInputDevice(new InputDevice("/dev/input/by-path/platform-rotary-event")),
    connman(new Connman(this)),
    machine(new Machine(this)),
    mediaCtl(new MediaCtl(0, this)),
    fring(new Fring()),
    kirby(new KirbyConnection(uri, this)),
    updater(new Updater(machine, this)),
    nfc(new Nfc(this)),
    nubbock(new Nubbock(this)),
    pendingWifiMessage(NULL),
    pendingWifiId(QString()),
    pendingUpdateCheckMessage(NULL)
{
    // Defaults
    fring->setLedOff(0);
    fring->setLedOff(1);
    mixer->setMasterVolume(0.0);

    // Updater logic
    QObject::connect(updater, &Updater::updateAvailable, this, [this](const QString &version) {
        qInfo(DaemonLog) << "New update available, version" << version;

        if (pendingUpdateCheckMessage) {
            pendingUpdateCheckMessage->setPayload(QJsonObject({{ "available", true }}));
            kirby->sendMessage(*pendingUpdateCheckMessage);
            delete pendingUpdateCheckMessage;
            pendingUpdateCheckMessage = NULL;
        }
    });

    QObject::connect(updater, &Updater::alreadyUpToDate, this, [this]() {
        qInfo(DaemonLog) << "Already up-to-date!";

        if (pendingUpdateCheckMessage) {
            pendingUpdateCheckMessage->setPayload(QJsonObject({{ "available", false }}));
            kirby->sendMessage(*pendingUpdateCheckMessage);
            delete pendingUpdateCheckMessage;
            pendingUpdateCheckMessage = NULL;
        }

        // FIXME: more checks should be met before boot is considered verified!
        machine->verifyBootConfig();
    });

    QObject::connect(updater, &Updater::checkFailed, this, [this]() {
        qInfo(DaemonLog) << "Update check failed!";

        if (pendingUpdateCheckMessage) {
            pendingUpdateCheckMessage->setResponseError(true);
            kirby->sendMessage(*pendingUpdateCheckMessage);
            delete pendingUpdateCheckMessage;
            pendingUpdateCheckMessage = NULL;
        }
    });

    QObject::connect(updater, &Updater::updateSucceeded, this, [this]() {
        qInfo(DaemonLog) << "Update succeeded!";
        KirbyMessage msg("policy/update/UPDATE_FINISHED",
                             QJsonObject{{ "updateSuccessful", true }});
    });

    QObject::connect(updater, &Updater::updateFailed, this, [this]() {
        qInfo(DaemonLog) << "Update failed!";
        KirbyMessage msg("policy/update/UPDATE_FINISHED",
                             QJsonObject{{ "updateSuccessful", false }});
    });

    QObject::connect(updater, &Updater::updateProgress, this, [this](float progress) {
        qInfo(DaemonLog) << "Updater progress:" << progress;
        KirbyMessage msg("policy/update/UPDATE_PROGRESS",
                             QJsonObject{{ "progress", progress }});
        kirby->sendMessage(msg);
    });

    // Accelerometer
    QObject::connect(accelerometer, &Accelerometer::orientationChanged, this, [this](Accelerometer::Orientation o) {
        qInfo(DaemonLog) << "Orientation changed to" << o;
        KirbyMessage msg("policy/orientation/CHANGED");

        switch (o) {
        case Accelerometer::Standing:
        default:
            msg.setPayload(QJsonObject{{ "orientation", "standing" }});
            nubbock->setTransform("90");
            break;

        case Accelerometer::Laying:
            msg.setPayload(QJsonObject{{ "orientation", "laying" }});
            nubbock->setTransform("270");
            break;

            kirby->sendMessage(msg);
        }
        });

    accelerometer->emitCurrent();

    // ALSA
    if (machine->getModel() == Machine::NEPOS1)
        mixer->setMasterScale(0.5f);

    qInfo(DaemonLog) << "Current master volume:" << mixer->getMasterVolume();

    // Input devices
    QObject::connect(rotaryInputDevice, &InputDevice::inputEvent, this, [this](int type, int code, int value) {
        if (type != EV_REL || code != REL_X)
            return;

        KirbyMessage msg(value > 0 ?
                                 "policy/rotary/CW" :
                                 "policy/rotary/CCW");
        kirby->sendMessage(msg);
    });

    // Connman connection
    QObject::connect(connman, &Connman::availableWifisUpdated, this, [this](const QJsonArray &list) {
        KirbyMessage msg("policy/wifi/SCAN_RESULT", list);
        kirby->sendMessage(msg);
    });

    QObject::connect(connman, &Connman::wifiChanged, this, [this](const QJsonObject &wifi, const QString &state) {
        if (wifi["kalamiId"].toString() != pendingWifiId)
            return;

        if (pendingWifiMessage) {
            bool send = false;

            if (wifi["online"].toBool()) {
                pendingWifiMessage->setResponseError(false);
                send = true;
            } else if (state == "failure") {
                pendingWifiMessage->setResponseError(true);
                send = true;
            }

            if (send) {
                kirby->sendMessage(*pendingWifiMessage);
                delete pendingWifiMessage;
                pendingWifiMessage = NULL;
            }
        }

        KirbyMessage msg("policy/wifi/STATE_CHANGED", wifi);
        kirby->sendMessage(msg);
    });

    QObject::connect(connman, &Connman::goneOnline, this, [this]() {
        qInfo(DaemonLog) << "We are now online!";
    });

    connman->start();

    // Websocket connection
    QObject::connect(kirby, &KirbyConnection::messageReceived, this, &Daemon::kirbyMessageReceived);

    // D-Bus connection
    QDBusConnection bus = QDBusConnection::systemBus();
    if (bus.isConnected())
        qInfo(DaemonLog) << "Connected to D-Bus as" << bus.baseService();
    else
        qWarning(DaemonLog) << "D-Bus connection failed:" << bus.lastError();

    // fring
    if (fring->initialize()) {
        QObject::connect(fring, &Fring::homeButtonChanged, this, [this](bool state) {
            KirbyMessage msg("policy/homebutton/STATE_CHANGED", QJsonObject {
                                     { "id", "home" },
                                     { "state", state },
                                 });
            kirby->sendMessage(msg);
        });

        QObject::connect(fring, &Fring::batteryStateChanged, this, [this](float level, float chargeCurrent) {
            KirbyMessage msg("policy/battery/STATE_CHANGED", QJsonObject {
                                     { "level", level },
                                     { "chargeCurrent", chargeCurrent },
                                 });
            kirby->sendMessage(msg);
        });

        QObject::connect(fring, &Fring::ambientLightChanged, this, [this](float value) {
            KirbyMessage msg("policy/battery/AMBIENT_LIGHT_CHANGED", QJsonObject {
                                     { "value", value },
                                 });
            kirby->sendMessage(msg);
        });

        QObject::connect(fring, &Fring::logMessageReceived, this, [this](const QString &message) {
            qInfo(DaemonLog) << "Message from fring:" << message;
        });

        machine->setDeviceSerial(fring->getDeviceSerial());
    }

    if (!mediaCtl->initialize())
        qWarning(DaemonLog) << "MediaCtl failed to initialize";

    mediaCtl->setConfig(0, MediaCtl::UYVY8_2X8_1920x1080);

    updater->check("latest");
}

void Daemon::cancelResponse(KirbyMessage **msg)
{
    KirbyMessage *m = *msg;
    *msg = NULL;

    if (!m)
        return;

    m->setResponseError(true);
    kirby->sendMessage(*m);
    delete m;
}

void Daemon::kirbyMessageReceived(const KirbyMessage &message)
{
    bool ret = true;
    QJsonObject payload = message.payload().toObject();

    if (message.type() == "policy/display/SET_BRIGHTNESS") {
        KirbyMessage *response = message.makeResponse();
        ret = displayBrightness->setBrightness(payload["value"].toDouble());
        response->setResponseError(!ret);
        kirby->sendMessage(*response);
        delete response;
    }

    if (message.type() == "policy/led/SET_STATE") {
        QJsonObject color = payload["color"].toObject();
        int id = payload["id"] == "videocall" ? 1 : 0;

        if (payload["mode"] == "off")
            ret = fring->setLedOff(id);

        if (payload["mode"] == "on")
            ret = fring->setLedOn(id, color["red"].toDouble(), color["green"].toDouble(), color["blue"].toDouble());

        if (payload["mode"] == "flash")
            ret = fring->setLedFlashing(id, color["red"].toDouble(), color["green"].toDouble(), color["blue"].toDouble(),
                                        payload["onPhase"].toDouble(), payload["offPhase"].toDouble());

        if (payload["mode"] == "pulse")
            ret = fring->setLedPulsating(id, color["red"].toDouble(), color["green"].toDouble(), color["blue"].toDouble(),
                                         payload["frequency"].toDouble());

        KirbyMessage *response = message.makeResponse();
        response->setResponseError(!ret);
        kirby->sendMessage(*response);
        delete response;
    }

    if (message.type() == "policy/volume/SET") {
        KirbyMessage *response = message.makeResponse();
        ret = mixer->setMasterVolume(payload["volume"].toDouble());
        response->setResponseError(!ret);
        kirby->sendMessage(*response);
        delete response;
    }

    if (message.type() == "policy/wifi/CONNECT") {
        cancelResponse(&pendingWifiMessage);
        pendingWifiId = payload["kalamiId"].toString();
        pendingWifiMessage = message.makeResponse();
        connman->connectToWifi(pendingWifiId, payload["passphrase"].toString());
    }

    if (message.type() == "policy/wifi/DISCONNECT") {
        cancelResponse(&pendingWifiMessage);
        connman->disconnectFromWifi(payload["kalamiId"].toString());
    }

    if (message.type() == "policy/update/CHECK") {
        cancelResponse(&pendingUpdateCheckMessage);
        pendingUpdateCheckMessage = message.makeResponse();
        updater->check(payload["channel"].toString());
    }

    if (message.type() == "policy/update/UPDATE") {
        KirbyMessage *response = message.makeResponse();
        ret = updater->install();
        response->setResponseError(!ret);
        kirby->sendMessage(*response);
        delete response;
    }

    if (message.type() == "policy/suspend/SUSPEND") {
        float v = displayBrightness->getBrightness();
        displayBrightness->setBrightness(0.0);
        machine->suspend();
        displayBrightness->setBrightness(v);

        KirbyMessage msg("policy/suspend/RESUMED");
        kirby->sendMessage(msg);
    }

    Q_UNUSED(ret);
}

Daemon::~Daemon()
{
}
