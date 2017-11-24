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
    polyphant(new PolyphantConnection(uri, this)),
    updater(new Updater(machine, this)),
    nfc(new Nfc(this)),
    nubbock(new Nubbock(this)),
    pendingWifiMessage(NULL),
    pendingWifiId(QString()),
    pendingUpdateCheckMessage(NULL)
{
    // Updater logic
    QObject::connect(updater, &Updater::updateAvailable, this, [this](const QString &version) {
        qInfo(DaemonLog) << "New update available, version" << version;

        if (pendingUpdateCheckMessage) {
            pendingUpdateCheckMessage->setPayload(QJsonObject({{ "available", true }}));
            polyphant->sendMessage(*pendingUpdateCheckMessage);
            delete pendingUpdateCheckMessage;
            pendingUpdateCheckMessage = NULL;
        }
    });

    QObject::connect(updater, &Updater::alreadyUpToDate, this, [this]() {
        qInfo(DaemonLog) << "Already up-to-date!";

        if (pendingUpdateCheckMessage) {
            pendingUpdateCheckMessage->setPayload(QJsonObject({{ "available", false }}));
            polyphant->sendMessage(*pendingUpdateCheckMessage);
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
            polyphant->sendMessage(*pendingUpdateCheckMessage);
            delete pendingUpdateCheckMessage;
            pendingUpdateCheckMessage = NULL;
        }
    });

    QObject::connect(updater, &Updater::updateSucceeded, this, [this]() {
        qInfo(DaemonLog) << "Update succeeded!";
        PolyphantMessage msg("policy/update/UPDATE_FINISHED",
                             QJsonObject{{ "updateSuccessful", true }});
    });

    QObject::connect(updater, &Updater::updateFailed, this, [this]() {
        qInfo(DaemonLog) << "Update failed!";
        PolyphantMessage msg("policy/update/UPDATE_FINISHED",
                             QJsonObject{{ "updateSuccessful", false }});
    });

    QObject::connect(updater, &Updater::updateProgress, this, [this](float progress) {
        qInfo(DaemonLog) << "Updater progress:" << progress;
        PolyphantMessage msg("policy/update/UPDATE_PROGRESS",
                             QJsonObject{{ "progress", progress }});
        polyphant->sendMessage(msg);
    });

    // Accelerometer
    QObject::connect(accelerometer, &Accelerometer::orientationChanged, this, [this](Accelerometer::Orientation o) {
        qInfo(DaemonLog) << "Orientation changed to" << o;
        switch (o) {
        case Accelerometer::Standing:
        default:
            nubbock->setTransform("90");
            break;
        case Accelerometer::Laying:
            nubbock->setTransform("270");
            break;
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

        PolyphantMessage msg(value > 0 ?
                                 "policy/rotary/CW" :
                                 "policy/rotary/CCW",
                             QJsonObject{});
        polyphant->sendMessage(msg);
    });

    // Connman connection
    QObject::connect(connman, &Connman::availableWifisUpdated, this, [this](const QJsonArray &list) {
        PolyphantMessage msg("policy/wifi/SCAN_RESULT", list);
        polyphant->sendMessage(msg);
    });

    QObject::connect(connman, &Connman::wifiChanged, this, [this](const QJsonObject &wifi) {
        if (wifi["kalamiId"].toString() != pendingWifiId)
            return;

        if (pendingWifiMessage) {
            bool send = false;

            if (wifi["state"].toString() == "online") {
                send = true;
            } else if (wifi["state"].toString() == "error") {
                pendingWifiMessage->setResponseError(true);
                send = true;
            }

            if (send) {
                polyphant->sendMessage(*pendingWifiMessage);
                delete pendingWifiMessage;
                pendingWifiMessage = NULL;
            }
        }

        PolyphantMessage msg("policy/wifi/STATE_CHANGED", wifi);
        polyphant->sendMessage(msg);
    });

    QObject::connect(connman, &Connman::goneOnline, this, [this]() {
        qInfo(DaemonLog) << "We are now online!";
    });

    connman->start();

    // Websocket connection
    QObject::connect(polyphant, &PolyphantConnection::messageReceived, this, &Daemon::polyphantMessageReceived);

    // D-Bus connection
    QDBusConnection bus = QDBusConnection::systemBus();
    if (bus.isConnected())
        qInfo(DaemonLog) << "Connected to D-Bus as" << bus.baseService();
    else
        qWarning(DaemonLog) << "D-Bus connection failed:" << bus.lastError();

    // fring
    if (fring->initialize()) {
        QObject::connect(fring, &Fring::homeButtonChanged, this, [this](bool state) {
            PolyphantMessage msg("policy/homebutton/STATE_CHANGED", QJsonObject {
                                     { "id", "home" },
                                     { "state", state },
                                 });
            polyphant->sendMessage(msg);
        });

        QObject::connect(fring, &Fring::batteryStateChanged, this, [this](float level, float chargeCurrent) {
            PolyphantMessage msg("policy/battery/STATE_CHANGED", QJsonObject {
                                     { "level", level },
                                     { "chargeCurrent", chargeCurrent },
                                 });
            polyphant->sendMessage(msg);
        });

        QObject::connect(fring, &Fring::ambientLightChanged, this, [this](float value) {
            PolyphantMessage msg("policy/battery/AMBIENT_LIGHT_CHANGED", QJsonObject {
                                     { "value", value },
                                 });
            polyphant->sendMessage(msg);
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

void Daemon::polyphantMessageReceived(const PolyphantMessage &message)
{
    bool ret = true;
    QJsonObject payload = message.payload().toObject();

    if (message.type() == "policy/display/SET_BRIGHTNESS") {
        PolyphantMessage *response = message.makeResponse();
        ret = displayBrightness->setBrightness(payload["value"].toDouble());
        response->setResponseError(!ret);
        polyphant->sendMessage(*response);
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

        PolyphantMessage *response = message.makeResponse();
        response->setResponseError(!ret);
        polyphant->sendMessage(*response);
        delete response;
    }

    if (message.type() == "policy/volume/SET") {
        PolyphantMessage *response = message.makeResponse();
        ret = mixer->setMasterVolume(payload["volume"].toDouble());
        response->setResponseError(!ret);
        polyphant->sendMessage(*response);
        delete response;
    }

    if (message.type() == "policy/wifi/CONNECT") {
        if (pendingWifiMessage) {
            pendingWifiMessage->setResponseError(true);
            polyphant->sendMessage(*pendingWifiMessage);
            delete pendingWifiMessage;
            pendingWifiMessage = NULL;
        }

        QString id = payload["kalamiId"].toString();

        connman->connectToWifi(id, payload["passphrase"].toString());
    }

    if (message.type() == "policy/wifi/DISCONNECT") {
        connman->disconnectFromWifi(payload["kalamiId"].toString());
    }

    if (message.type() == "policy/update/CHECK") {
        if (pendingUpdateCheckMessage) {
            pendingUpdateCheckMessage->setResponseError(true);
            polyphant->sendMessage(*pendingUpdateCheckMessage);
            delete pendingUpdateCheckMessage;
            pendingUpdateCheckMessage = NULL;
        }

        updater->check(payload["channel"].toString());
    }

    Q_UNUSED(ret);
}

Daemon::~Daemon()
{
}
