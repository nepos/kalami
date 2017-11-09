#include "polyphantmessage.h"

PolyphantMessage::PolyphantMessage(const QJsonObject json)
{
    _type = json["type"].toString();
    _payload = json["payload"].toObject();
    _meta = json["meta"].toObject();
}

PolyphantMessage::PolyphantMessage(const QString type, const QJsonValue payload, int requestId, const QJsonObject meta) :
    _type(type),
    _payload(payload),
    _meta(meta)
{
    _meta["requestId"] = requestId;
}

const QJsonObject PolyphantMessage::toJson() const {
    return QJsonObject {
        { "type", _type, },
        { "payload", _payload, },
        { "meta", _meta },
    };
}

PolyphantMessage PolyphantMessage::makeResponse(const QJsonValue payload) const
{
    QJsonObject meta({
                         { "commType", "response" },
                     });

    return PolyphantMessage(_type, payload, _meta["requestId"].toInt(), meta);
}
