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

void PolyphantMessage::setPayload(const QJsonValue &payload)
{
    _payload = payload;
}

void PolyphantMessage::setType(const QString &string)
{
    _type = string;
}

const QJsonObject PolyphantMessage::toJson() const {
    QJsonObject o({
                      { "type", _type }
                  });

    if (!_payload.isNull())
        o["payload"] = _payload;

    if (!_meta.isEmpty())
        o["meta"] = _meta;

    return o;
}

PolyphantMessage* PolyphantMessage::makeResponse(const QJsonValue payload) const
{
    QJsonObject meta({
                         { "commType", "response" },
                     });

    return new PolyphantMessage(_type, payload, _meta["requestId"].toInt(), meta);
}
