#pragma once

#include <QJsonObject>

class KirbyMessage
{
public:
    explicit KirbyMessage(const QJsonObject json);
    explicit KirbyMessage(const QString type, const QJsonValue payload = {}, const QJsonObject meta = {});

    const QString type() const { return _type; };
    const QString messageId() const { return _payload.toObject()["id"].toString(); };
    const QJsonValue payload() const { return _payload; };
    const QString metaPending() const { return _meta["pending"].toString(); };
    const QString metaSuccess() const { return _meta["success"].toString(); };
    const QString metaError() const { return _meta["error"].toString(); };
    int requestId() const { return _meta["requestId"].toInt(); };

    void setPayload(const QJsonValue &payload);
    void setResponseError(bool error);
    const QJsonObject toJson() const;

    KirbyMessage* makeResponse() const;

private:
    QString _type;
    QJsonValue _payload;
    QJsonObject _meta;
};
