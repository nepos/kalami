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
#include <QJsonArray>
#include <QJsonDocument>
#include "polyphantconnection.h"

Q_LOGGING_CATEGORY(PolyphantConnectionLog, "PolyphantConnection")

PolyphantConnection::PolyphantConnection(const QUrl &uri, QObject *parent) :
    QObject(parent), socket()
{
    QObject::connect(&socket, &QWebSocket::connected, [this]() {
        qInfo(PolyphantConnectionLog) << "Now connected to polyphant at" << socket.requestUrl();
    });

    QObject::connect(&socket, &QWebSocket::disconnected, [this, uri]() {
        qWarning(PolyphantConnectionLog) << "Polyphant disconnected, trying to reconnect ...";

        QTimer::singleShot(1000, [this, uri]() {
            socket.open(uri);
        });
    });

    QObject::connect(&socket, &QWebSocket::textMessageReceived, [this](const QString &message) {
        QJsonDocument doc = QJsonDocument::fromJson(message.toLocal8Bit());

        if (doc.isObject()) {
            const PolyphantMessage pmessage(doc.object());
            emit messageReceived(pmessage);
        }
    });

    socket.open(uri);
}

void PolyphantConnection::sendMessage(const PolyphantMessage &message)
{
    const QJsonObject obj = message.toJson();
    QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    qInfo() << "XXX SENDING TO POLYPHANT:" << QString(payload);
    socket.sendBinaryMessage(payload);
}

