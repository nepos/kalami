#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include "updater.h"

Updater::Updater(const Machine *machine, const QString &updateChannel, QObject *parent) :
    QObject(parent), machine(machine), updateChannel(updateChannel), networkAccessManager(this)
{
}

void Updater::check()
{
    QUrlQuery query;
    QString currentVersion = QString::number(machine->getOsVersion());

    query.addQueryItem("current", currentVersion);

    QUrl url("http://os.nepos.io/updates/" + updateChannel + ".json");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader(QString("X-nepos-current").toLocal8Bit(), currentVersion.toLocal8Bit());
//    request.setRawHeader(QString("X-nepos-machine-id").toLocal8Bit(), currentVersion.toLocal8Bit());
//    request.setRawHeader(QString("X-nepos-device-model").toLocal8Bit(), currentVersion.toLocal8Bit());
//    request.setRawHeader(QString("X-nepos-device-revision").toLocal8Bit(), currentVersion.toLocal8Bit());
//    request.setRawHeader(QString("X-nepos-device-serial").toLocal8Bit(), currentVersion.toLocal8Bit());

    QNetworkReply *reply = networkAccessManager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = (QNetworkReply *) sender();
        QByteArray content = reply->readAll();

        qDebug() << "content" << content;
    });
}
