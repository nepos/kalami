/***
  Copyright (c) 2016 Nepos GmbH

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

#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include <QWebSocket>
#include <QtDBus/QDBusInterface>

#include "udevmonitor.h"
#include "wpasupplicant.h"

class Daemon : public QObject
{
    Q_OBJECT
public:
    explicit Daemon(QUrl serverUri, QObject *parent = 0);
    ~Daemon();

signals:

public slots:

private:
    QUrl serverUri;
    QWebSocket *socket;

    UDevMonitor *udev;
    WpaSupplicant *wpaSupplicant;
    QDBusInterface *systemdConnection;

    void dispatchSocketMessage(const QJsonValue &type, const QJsonValue &element, const QJsonValue &value);
    void sendSocketMessage(const QString &type, const QString &element, const QString &value);
};

#endif // DAEMON_H
