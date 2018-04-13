#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>

#include "nubbock.h"

Q_LOGGING_CATEGORY(NubbockLog, "Nubbock")

Nubbock::Nubbock(QObject *parent) :
    QObject(parent),
    transform(),
    socket(this)
{
    endpoint = "/run/nubbock/socket";

    QObject::connect(&socket, &QLocalSocket::connected, [this]() {
        qInfo(NubbockLog) << "Now connected to Nubbock at" << socket.serverName();
        sendState();
    });

    QObject::connect(&socket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),
          [=](QLocalSocket::LocalSocketError socketError){
        Q_UNUSED(socketError);

        qWarning(NubbockLog) << "Nubbock disconnected, trying to reconnect ...";

        QTimer::singleShot(1000, [this]() {
            socket.connectToServer(endpoint);
        });
    });

    socket.connectToServer(endpoint);
}

bool Nubbock::setTransform(enum Transform t)
{
    transform = t;
    return sendState();
}

void Nubbock::suspend()
{
    suspended = true;
    sendState();
}

void Nubbock::resume()
{
    suspended = false;
    sendState();
}

bool Nubbock::sendState(void)
{
    if (!socket.isWritable())
        return false;

    QJsonObject obj({
                        { "transform", transform == Nubbock::TRANSFORM_90 ? "90" : "270" },
                        { "suspended", suspended },
                    });

    QByteArray ba = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    qint64 r = socket.write(ba.constData(), ba.length() + 1);

    return r == ba.length();
}
