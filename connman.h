#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(ConnmanLog)

class ConnmanPrivate;
class NetworkService;

class Connman : public QObject
{
    Q_OBJECT
public:
    explicit Connman(QObject *parent = 0);
    void start();
    bool connectToWifi(const QString &wifiId, const QString &passphrase, bool iterateImmediately = true);
    bool disconnectFromWifi(const QString &wifiId);
    void suspend();
    void resume();

signals:
    void availableWifisUpdated(const QJsonArray &list);
    void wifiChanged(const QJsonObject &wifi, const QString &state);

private slots:
    bool disconnectFromWifiById(const QString &wifiId);
    void iterateServices();
    void enableWifi();

private:
    ConnmanPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Connman);
    QString kalamiIdForService(const NetworkService *service);
};
