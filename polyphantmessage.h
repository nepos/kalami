#ifndef POLYPHANTMESSAGE_H
#define POLYPHANTMESSAGE_H

#include <QJsonObject>

class PolyphantMessage
{
public:
    explicit PolyphantMessage(const QJsonObject json);
    explicit PolyphantMessage(const QString type, const QJsonValue payload, int requestId, const QJsonObject meta = {});

    const QString type() const { return _type; };
    const QString messageId() const { return _payload.toObject()["id"].toString(); };
    const QJsonValue payload() const { return _payload; };
    const QString metaPending() const { return _meta["pending"].toString(); };
    const QString metaSuccess() const { return _meta["success"].toString(); };
    const QString metaError() const { return _meta["error"].toString(); };
    int requestId() const { return _meta["requestId"].toInt(); };

    void setPayload(const QJsonValue &payload);
    void setResponseSuccess(bool success);
    const QJsonObject toJson() const;

    PolyphantMessage* makeResponse(const QJsonValue payload = {}) const;

private:
    QString _type;
    QJsonValue _payload;
    QJsonObject _meta;
    int _requestId;
};

#endif // POLYPHANTMESSAGE_H
