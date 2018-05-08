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
    pendingUpdateCheckMessage(NULL),
    pendingBootstrapInternalMessage(NULL)
{
    // Defaults
    fring->setLedOff(0);
    fring->setLedOff(1);
    mixer->setMasterVolume(0.0);

    //Machine
    QObject::connect(machine, &Machine::bootstrapInternalMemoryFinished, [this](bool success) {
        if (pendingBootstrapInternalMessage) {
            pendingBootstrapInternalMessage->setResponseError(!success);
            kirby->sendMessage(*pendingBootstrapInternalMessage);
            delete pendingBootstrapInternalMessage;
            pendingBootstrapInternalMessage = NULL;
        }
    });

    // Updater logic
    QObject::connect(updater, &Updater::updateAvailable, [this](const QString &version) {
        qInfo(DaemonLog) << "New update available, version" << version;

        if (pendingUpdateCheckMessage) {
            pendingUpdateCheckMessage->setPayload(QJsonObject({{ "available", true }}));
            kirby->sendMessage(*pendingUpdateCheckMessage);
            delete pendingUpdateCheckMessage;
            pendingUpdateCheckMessage = NULL;
        }
    });

    QObject::connect(updater, &Updater::alreadyUpToDate, [this]() {
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

    QObject::connect(updater, &Updater::checkFailed, [this]() {
        qInfo(DaemonLog) << "Update check failed!";

        if (pendingUpdateCheckMessage) {
            pendingUpdateCheckMessage->setResponseError(true);
            kirby->sendMessage(*pendingUpdateCheckMessage);
            delete pendingUpdateCheckMessage;
            pendingUpdateCheckMessage = NULL;
        }
    });

    QObject::connect(updater, &Updater::updateSucceeded, [this]() {
        qInfo(DaemonLog) << "Update succeeded!";
        KirbyMessage msg("policy/update/UPDATE_FINISHED",
                         QJsonObject{{ "updateSuccessful", true }});
    });

    QObject::connect(updater, &Updater::updateFailed, [this]() {
        qInfo(DaemonLog) << "Update failed!";
        KirbyMessage msg("policy/update/UPDATE_FINISHED",
                         QJsonObject{{ "updateSuccessful", false }});
    });

    QObject::connect(updater, &Updater::updateProgress, [this](float progress) {
        qInfo(DaemonLog) << "Updater progress:" << progress;
        KirbyMessage msg("policy/update/UPDATE_PROGRESS",
                         QJsonObject{{ "progress", progress }});
        kirby->sendMessage(msg);
    });

    // Accelerometer
    QObject::connect(accelerometer, &Accelerometer::orientationChanged, [this](Accelerometer::Orientation o) {
        qInfo(DaemonLog) << "Orientation changed to" << o;
        KirbyMessage msg("policy/orientation/CHANGED");

        switch (o) {
        case Accelerometer::Standing:
        default:
            msg.setPayload(QJsonObject{{ "orientation", "standing" }});
            break;

        case Accelerometer::Laying:
            msg.setPayload(QJsonObject{{ "orientation", "laying" }});
            break;
        }

        kirby->sendMessage(msg);
    });

    accelerometer->emitCurrent();

    // ALSA
    if (machine->getModel() == Machine::NEPOS1)
        mixer->setMasterScale(0.5f);

    // Input devices
    QObject::connect(rotaryInputDevice, &InputDevice::inputEvent, [this](int type, int code, int value) {
        if (type != EV_REL || code != REL_X)
            return;

        KirbyMessage msg(value > 0 ?
                             "policy/rotary/CW" :
                             "policy/rotary/CCW");
        kirby->sendMessage(msg);
    });

    // Connman connection
    QObject::connect(connman, &Connman::availableWifisUpdated, [this](const QJsonArray &list) {
        KirbyMessage msg("policy/wifi/SCAN_RESULT", list);
        kirby->sendMessage(msg);
    });

    QObject::connect(connman, &Connman::wifiChanged, [this](const QJsonObject &wifi, const QString &state) {
        if (wifi["kalamiId"].toString() != pendingWifiId)
            return;

        qInfo(DaemonLog) << "reporting wifiChanged. pending" << pendingWifiMessage;

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

    connman->start();

    // Websocket connection
    QObject::connect(kirby, &KirbyConnection::connected, this, &Daemon::sendDeviceInformation);
    QObject::connect(kirby, &KirbyConnection::messageReceived, this, &Daemon::kirbyMessageReceived);

    // D-Bus connection
    QDBusConnection bus = QDBusConnection::systemBus();
    if (bus.isConnected())
        qInfo(DaemonLog) << "Connected to D-Bus as" << bus.baseService();
    else
        qWarning(DaemonLog) << "D-Bus connection failed:" << bus.lastError();

    // fring
    if (fring->initialize()) {
        QObject::connect(fring, &Fring::homeButtonChanged, [this](bool state) {
            KirbyMessage msg("policy/homebutton/STATE_CHANGED", QJsonObject {
                                 { "id", "home" },
                                 { "state", state },
                             });
            kirby->sendMessage(msg);
        });

        QObject::connect(fring, &Fring::batteryStateChanged, [this](float level, float chargeCurrent, float temperature, float timeToEmpty, float timeToFull) {
            KirbyMessage msg("policy/battery/STATE_CHANGED", QJsonObject {
                                 { "level", level },
                                 { "chargingCurrent", chargeCurrent },
                                 { "temperature", temperature },
                                 { "timeToEmpty", timeToEmpty },
                                 { "timeToFull", timeToFull },
                             });
            kirby->sendMessage(msg);
        });

        QObject::connect(fring, &Fring::ambientLightChanged, [this](float value) {
            KirbyMessage msg("policy/battery/AMBIENT_LIGHT_CHANGED", QJsonObject {
                                 { "value", value },
                             });
            kirby->sendMessage(msg);
        });

        QObject::connect(fring, &Fring::wakeupReasonChanged, [this](Fring::WakeupReason reason) {

            QString strReason = "unknown";
            //TODO: How do we behave in these cases?!
            if (reason == Fring::WAKEUP_REASON_RTC) {
                strReason = "rtc";
                connman->resume();

            } else if (reason == Fring::WAKEUP_REASON_HOMEBUTTON) {
                strReason = "homebutton";
                connman->resume();
                displayBrightness->resume();
                nubbock->resume();

            }

            KirbyMessage msg("policy/power-management/RESUMED", QJsonObject {
                                 { "wakeupReason", strReason },
                             });

            qInfo(DaemonLog) << "Wakeup reason: " << strReason;

            kirby->sendMessage(msg);
        });

        QObject::connect(fring, &Fring::logMessageReceived, [this](const QString &message) {
            qInfo(DaemonLog) << "Message from fring:" << message;
        });

        machine->setDeviceSerial(fring->getDeviceSerial());
    }

    if (!mediaCtl->initialize())
        qWarning(DaemonLog) << "MediaCtl failed to initialize";

    mediaCtl->setConfig(0, MediaCtl::UYVY8_2X8_1920x1080);
    mediaCtl->setConfig(1, MediaCtl::UYVY8_2X8_1920x1080);
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

void Daemon::sendDeviceInformation()
{
    KirbyMessage msg("policy/device_information/DEVICE_INFORMATION",
                     QJsonObject {
                         { "model", machine->getModelName() },
                         { "osVersion", QString::number(machine->getOsVersionNumber()) },
                         { "osChannel", machine->getOsChannel() },
                         { "bootSource", (machine->getBootSource() == Machine::BOOTSOURCE_INTERNAL ? "internal" : "external") },
                         { "bootConfig", (machine->getBootConfig() == Machine::BOOTCONFIG_A ? "A" : "B") },
                         { "tentativeBoot", machine->isTentativeBoot() },
                         { "firmwareVersion", fring->getFirmwareVersion() },
                         { "hardwareErrors", QString::number(fring->getHardwareErrors()) },
                         { "deviceSerial", fring->getDeviceSerial() },
                         { "boardRevisionA", fring->getBoardRevisionA() },
                         { "boardRevisionB", fring->getBoardRevisionB() },
                     });

        kirby->sendMessage(msg);
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

    if (message.type() == "policy/display/SET_ROTATION") {
        KirbyMessage *response = message.makeResponse();
        int rotation = payload["value"].toInt();

        ret = nubbock->setTransform(rotation == 0 ?
                                        Nubbock::TRANSFORM_90 :
                                        Nubbock::TRANSFORM_270);
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
        QString id = payload["kalamiId"].toString();
        if (id == pendingWifiId)
            pendingWifiId.clear();

        connman->disconnectFromWifi(id);
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

    if (message.type() == "policy/power-management/SHUTDOWN") {
        machine->powerOff();
    }

    if (message.type() == "policy/power-management/REBOOT") {
        machine->restart();
    }

    if (message.type() == "policy/power-management/SUSPEND") {
        int wakeupMs = 0;

        if (payload.contains("wakeupMs"))
            wakeupMs = payload["wakeupMs"].toInt();

        // Set wakeuptime in any case (0 = disabled), since fring also sets
        // its som_state to SUSPENDED, based on that call.
        fring->setWakeupMs(wakeupMs);

        connman->suspend();
        nubbock->suspend();
        displayBrightness->suspend();

        // Give Nubbock and the display backlight regulator some time to settle
        QThread::sleep(1);

        // This will actually put the kernel to sleep
        machine->suspend();

        // For resume, see slot wakeupReasonChanged.
    }

    if (message.type() == "policy/bootstrap/BOOTSTRAP_INTERNAL_MEMORY") {
        cancelResponse(&pendingBootstrapInternalMessage);
        pendingBootstrapInternalMessage = message.makeResponse();
    }

    Q_UNUSED(ret);
}

Daemon::~Daemon()
{
}
