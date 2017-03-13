#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include "updater.h"

Updater::Updater(const Machine *machine, const QString &updateChannel, QObject *parent) :
    QObject(parent), machine(machine), updateChannel(updateChannel), networkAccessManager(this)
{
}

Updater::~Updater()
{
    if (pendingReply)
        pendingReply->abort();
}

void Updater::check()
{
    QUrlQuery query;
    QString currentVersion = QString::number(machine->getOsVersion());

    query.addQueryItem("current", currentVersion);

    QUrl url("http://os.nepos.io/updates/" + updateChannel + ".json");
    url.setQuery(query);

    qInfo() << "Checking for updates on" << url;

    QNetworkRequest request(url);
    request.setRawHeader(QString("X-nepos-current").toLocal8Bit(), currentVersion.toLocal8Bit());
    request.setRawHeader(QString("X-nepos-machine-id").toLocal8Bit(), machine->getMachineId().toLocal8Bit());
    request.setRawHeader(QString("X-nepos-device-model").toLocal8Bit(), machine->getModelName().toLocal8Bit());
    request.setRawHeader(QString("X-nepos-device-revision").toLocal8Bit(), machine->getDeviceRevision().toLocal8Bit());
    request.setRawHeader(QString("X-nepos-device-serial").toLocal8Bit(), machine->getDeviceSerial().toLocal8Bit());

    if (pendingReply)
        pendingReply->abort();

    pendingReply = networkAccessManager.get(request);
    QObject::connect(pendingReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = (QNetworkReply *) sender();
        QByteArray content = reply->readAll();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(content, &error);

        if (error.error != QJsonParseError::NoError) {
            qWarning() << "Unable to parse Json content from update server:" << error.errorString();
            return;
        }

        if (!doc.isObject()) {
            qWarning() << "Huh!? No content from update server?";
            return;
        }

        QJsonObject json = doc.object();
        availableUpdate.version = json["build_id"].toString().toULong();
        availableUpdate.rootfsUrl = QUrl(json["rootfs"].toString());
        availableUpdate.bootimgUrl = QUrl(json["bootimg"].toString());

        if (availableUpdate.version < machine->getOsVersion())
            emit updateAvailable(QString::number(availableUpdate.version));
    });
}

int Updater::installAvailableUpdate()
{
    if (availableUpdate->version == 0)
        return -ENOENT;

    // TODO

    return 0;
}
