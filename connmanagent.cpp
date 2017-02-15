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

#include "connmanagent.h"

ConnmanAgent::ConnmanAgent(QObject *parent) :
    QObject(parent), adaptor(this)
{
}

void ConnmanAgent::Cancel()
{
    qInfo() << "CANCEL";
}

void ConnmanAgent::Release()
{
    qInfo() << "RELEASE";
}

void ConnmanAgent::ReportError(const QDBusObjectPath &service, const QString &error)
{
    qInfo() << "ERROR";
}

void ConnmanAgent::RequestBrowser(const QDBusObjectPath &service, const QString &url)
{
    qInfo() << "BROWSER";
}

QVariantMap ConnmanAgent::RequestInput(const QDBusObjectPath &servicePath, const QVariantMap &fields)
{
    qInfo() << "REQUESTINPUT";

    QVariantMap response;

    if (fields.keys().contains("Passphrase"))
        response["Passphrase"] = "beineschwacke";

    if (fields.keys().contains("SSID") || fields.keys().contains("Name")) {
        response["SSID"] = "Zambaramba";
        response["Name"] = "Zambaramba";
    }

    return response;
}
