#include "polyphantmessage.h"

PolyphantMessage::PolyphantMessage(const QJsonObject json)
{
    _type = json["type"].toString();
    _payload = json["payload"].toObject();
    _meta = json["meta"].toObject();
}

PolyphantMessage::PolyphantMessage(const QString type, const QJsonValue payload, const QJsonObject meta) :
    _type(type),
    _payload(payload),
    _meta(meta)
{
    if (!_meta["commType"].isString())
        _meta["commType"] = "one-way";
}

void PolyphantMessage::setPayload(const QJsonValue &payload)
{
    _payload = payload;
}

void PolyphantMessage::setResponseError(bool error)
{
    _meta["error"] = error;
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

PolyphantMessage* PolyphantMessage::makeResponse() const
{
    QJsonObject meta({
                         { "commType", "response" },
                         { "requestId", _meta["requestId"]},
                         { "destination", _meta["source"] },
                         { "source", "KALAMI" }
                     });

    return new PolyphantMessage(_type, QJsonObject(), meta);
}
