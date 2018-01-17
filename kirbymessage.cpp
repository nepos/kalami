#include "kirbymessage.h"

KirbyMessage::KirbyMessage(const QJsonObject json)
{
    _type = json["type"].toString();
    _payload = json["payload"].toObject();
    _meta = json["meta"].toObject();
}

KirbyMessage::KirbyMessage(const QString type, const QJsonValue payload, const QJsonObject meta) :
    _type(type),
    _payload(payload),
    _meta(meta)
{
    if (!_meta["commType"].isString())
        _meta["commType"] = "one-way";

    if (!_meta["destination"].isString())
        _meta["destination"] = "POLYPHANT_CLIENT";
}

void KirbyMessage::setPayload(const QJsonValue &payload)
{
    _payload = payload;
}

void KirbyMessage::setResponseError(bool error)
{
    _meta["error"] = error;
}

const QJsonObject KirbyMessage::toJson() const {
    QJsonObject o({
                      { "type", _type }
                  });

    if (!_payload.isNull())
        o["payload"] = _payload;

    if (!_meta.isEmpty())
        o["meta"] = _meta;

    return o;
}

KirbyMessage* KirbyMessage::makeResponse() const
{
    QJsonObject meta({
                         { "commType", "response" },
                         { "requestId", _meta["requestId"]},
                         { "destination", _meta["source"] },
                         { "source", "KALAMI" }
                     });

    return new KirbyMessage(_type, QJsonObject(), meta);
}
