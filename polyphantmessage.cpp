#include "polyphantmessage.h"

PolyphantMessage::PolyphantMessage(const QJsonObject json, QObject *parent) : QObject(parent)
{
    _type = json["error"].toString();
    _payload = json["payload"].toObject();
    _meta = json["meta"].toObject();
}

PolyphantMessage::PolyphantMessage(const QString type, const QJsonObject payload, const QJsonObject meta, QObject *parent) :
    QObject(parent),
    _type(type),
    _payload(payload),
    _meta(meta)
{}

const QJsonObject PolyphantMessage::toJson() const {
    return QJsonObject {
        { "type", _type, },
        { "payload", _payload, },
        { "meta", _meta },
    };
}

