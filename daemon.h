/***
  Copyright (c) 2017 Nepos GmbH

  Authors: Daniel Mack <daniel@nepos.io>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this software; If not, see <http://www.gnu.org/licenses/>.
***/

#pragma once

#include <QObject>
#include <QWebSocket>
#include <QtDBus/QDBusInterface>
#include <QtCore/QLoggingCategory>

#include "alsamixer.h"
#include "connman.h"
#include "machine.h"
#include "polyphantconnection.h"
#include "udevmonitor.h"
#include "updater.h"
#include "nfc.h"
#include "gpio.h"

Q_DECLARE_LOGGING_CATEGORY(DaemonLog)

class Daemon : public QObject
{
    Q_OBJECT
public:
    explicit Daemon(QUrl serverUri, QObject *parent = 0);
    ~Daemon();

signals:

public slots:

private slots:
    void polyphantMessageReceived(const PolyphantMessage &message);

private:
    ALSAMixer *mixer;
    Updater *updater;
    Connman *connman;
    Machine *machine;
    QDBusInterface *systemdConnection;
    PolyphantConnection *polyphant;
    UDevMonitor *udev;
    Nfc *nfc;
    GPIO *gpio;
};
