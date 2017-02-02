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

}

void ConnmanAgent::ReportError(const QDBusObjectPath &in0, const QString &in1)
{

}

void ConnmanAgent::RequestBrowser(const QDBusObjectPath &in0, const QString &in1)
{

}

QVariantMap ConnmanAgent::RequestInput(const QDBusObjectPath &in0, const QVariantMap &in1)
{

}
