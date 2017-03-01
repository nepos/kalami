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
#include "reduxproxy.h"

ReduxProxy::ReduxProxy(const QUrl &uri, const QStringList &filter, QObject *parent) :
    QObject(parent), socket()
{
    QObject::connect(&socket, &QWebSocket::connected, [this, filter]() {
        QJsonObject msg = {
            { "subscribe", QJsonArray::fromStringList(filter) },
        };

        sendJson(msg);

        qInfo() << "Redux proxy now connected to" << socket.requestUrl();
    });

    QObject::connect(&socket, &QWebSocket::disconnected, [this, uri]() {
        qInfo() << "Redux proxy disconnected, trying to reconnect ...";

        QTimer::singleShot(1000, [this, uri]() {
            socket.open(uri);
        });
    });

    QObject::connect(&socket, &QWebSocket::textMessageReceived, [this](const QString &message) {
        QJsonDocument doc = QJsonDocument::fromJson(message.toLocal8Bit());

        if (doc.isObject()) {
            QJsonObject json = doc.object();
            if (json.contains("state"))
                emit stateUpdated(json["state"].toObject());
        }
    });

    socket.open(uri);
}

void ReduxProxy::sendJson(const QJsonObject &msg)
{
    QByteArray payload = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    socket.sendBinaryMessage(payload);
}

void ReduxProxy::dispatchAction(const QJsonObject &action)
{
    QJsonObject msg = {
        { "action", action },
    };

    sendJson(msg);
}
