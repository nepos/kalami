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

#include <QTimer>

#include "daemon.h"

Daemon::Daemon(QUrl uri, QObject *parent) :
    QObject(parent), serverUri(uri)
{
    socket = new QWebSocket();

    QObject::connect(socket, &QWebSocket::connected, [this]() {
    });

    QObject::connect(socket, &QWebSocket::disconnected, [this]() {
        QTimer::singleShot(500, [this]() {
            socket->open(serverUri);
        });
    });

    socket->open(serverUri);
}

Daemon::~Daemon()
{
    delete socket;
}
