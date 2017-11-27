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
#include "kirbyconnection.h"

Q_LOGGING_CATEGORY(KirbyConnectionLog, "KirbyConnection")

KirbyConnection::KirbyConnection(const QUrl &uri, QObject *parent) :
    QObject(parent), socket()
{
    QObject::connect(&socket, &QWebSocket::connected, [this]() {
        qInfo(KirbyConnectionLog) << "Now connected to Kirby at" << socket.requestUrl();
    });

    QObject::connect(&socket, &QWebSocket::disconnected, [this, uri]() {
        qWarning(KirbyConnectionLog) << "Kirby disconnected, trying to reconnect ...";

        QTimer::singleShot(1000, [this, uri]() {
            socket.open(uri);
        });
    });

    QObject::connect(&socket, &QWebSocket::textMessageReceived, [this](const QString &message) {
        qInfo(KirbyConnectionLog) << "<" << QString(message);

        QJsonDocument doc = QJsonDocument::fromJson(message.toLocal8Bit());

        if (doc.isObject()) {
            const KirbyMessage pmessage(doc.object());
            emit messageReceived(pmessage);
        }
    });

    socket.open(uri);
}

void KirbyConnection::sendMessage(const KirbyMessage &message)
{
    const QJsonObject obj = message.toJson();
    qInfo(KirbyConnectionLog) << ">" << obj;
    QByteArray ba = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    socket.sendBinaryMessage(ba);
}

