#ifndef POLYPHANTMESSAGE_H
#define POLYPHANTMESSAGE_H

#include <QObject>
#include <QJsonObject>

class PolyphantMessage : public QObject
{
    Q_OBJECT
public:
    explicit PolyphantMessage(const QJsonObject json, QObject *parent = 0);
    explicit PolyphantMessage(const QString type, const QJsonObject payload, int requestId, const QJsonObject meta = {}, QObject *parent = 0);

    const QString type() const { return _type; };
    const QString messageId() const { return _payload["id"].toString(); };
    const QJsonObject payload() const { return _payload; };
    const QString metaPending() const { return _meta["pending"].toString(); };
    const QString metaSuccess() const { return _meta["success"].toString(); };
    const QString metaError() const { return _meta["error"].toString(); };
    int requestId() const { return _meta["requestId"].toInt(); };

    const QJsonObject toJson() const;

private:
    QString _type;
    QJsonObject _payload;
    QJsonObject _meta;
};

#endif // POLYPHANTMESSAGE_H
