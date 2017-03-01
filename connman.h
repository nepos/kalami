#ifndef CONNMAN_H
#define CONNMAN_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

class ConnmanPrivate;

class Connman : public QObject
{
    Q_OBJECT
public:
    explicit Connman(QObject *parent = 0);
    void updateKnownWifis(QJsonArray &list);

signals:
    void availableWifisUpdated(QJsonArray &list);

private slots:
    void iterateServices();
    void connectToKnownWifi();
    void agentPassphraseRequested();

private:
    ConnmanPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Connman);
};

#endif // CONNMAN_H
