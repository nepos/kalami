#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QProcess>
#include <QFile>
#include <QEventLoop>
#include <QTimer>
#include <QFileInfo>
#include "updater.h"

Q_LOGGING_CATEGORY(UpdaterLog, "Updater")

Updater::Updater(const Machine *machine, const QString &updateChannel, QObject *parent) :
    QObject(parent), machine(machine), updateChannel(updateChannel), networkAccessManager(this)
{
    pendingReply = NULL;
    thread = NULL;
}

Updater::~Updater()
{
    if (pendingReply)
        pendingReply->abort();
}

const QString &Updater::getUpdateSeed(Updater::ImageType type) const
{
    switch (type) {
    case Updater::IMAGE_TYPE_BOOTIMG:
        return machine->getAltBootDevice();

    case Updater::IMAGE_TYPE_ROOTFS:
        return machine->getAltRootfsDevice();
    }

    return NULL;
}

const QString &Updater::getUpdateTarget(Updater::ImageType type) const
{
    switch (type) {
    case Updater::IMAGE_TYPE_BOOTIMG:
        return machine->getCurrentBootDevice();

    case Updater::IMAGE_TYPE_ROOTFS:
        return machine->getCurrentRootfsDevice();
    }

    return NULL;
}

void Updater::check()
{
    QUrlQuery query;
    QString currentVersion = QString::number(machine->getOsVersion());
    QString model;

    switch (machine->getModel()) {
    case Machine::DT410C_EVALBOARD:
    case Machine::SAPHIRA:
        model = "saphira";
        break;

    default:
        model = "unknown";
    }

    query.addQueryItem("current", currentVersion);

    QUrl url("http://os.nepos.io/updates/" + model + "/" + updateChannel + ".json");
    url.setQuery(query);

    qInfo(UpdaterLog) << "Checking for updates on" << url;

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
            emit checkFailed("Unable to parse Json content from update server:" + error.errorString());
            return;
        }

        QJsonObject json = doc.object();
        availableUpdate.version = json["build_id"].toString().toULong();
        availableUpdate.rootfsUrl = QUrl(json["rootfs"].toString());
        availableUpdate.bootimgUrl = QUrl(json["bootimg"].toString());
        availableUpdate.casyncStore = QUrl(json["casync_store"].toString());

        if (availableUpdate.version > machine->getOsVersion())
            emit updateAvailable(QString::number(availableUpdate.version));
        else
            emit alreadyUpToDate();
    });
}

void Updater::install()
{
    if (availableUpdate.version == 0) {
        emit updateFailed();
        return;
    }

    if (thread) {
        thread->quit();
        thread->deleteLater();
        thread = NULL;
    }

    thread = new UpdateThread(this);

    QObject::connect(thread, &UpdateThread::succeeded, this, [this]() {
        emit updateSucceeded();
    });

    QObject::connect(thread, &UpdateThread::failed, this, [this]() {
        emit updateFailed();
    });

    thread->start();
}

UpdateThread::UpdateThread(const Updater *updater, QObject *parent) : QThread(parent), updater(updater)
{
}

bool UpdateThread::downloadFile(const QUrl &url, const QString &path)
{
    QNetworkAccessManager networkAccessManager;
    QNetworkRequest request(url);
    QNetworkReply *reply = networkAccessManager.get(request);
    QEventLoop loop;
    QTimer timer;
    bool ret = false;

    timer.setSingleShot(true);

    qDebug(UpdaterLog) << "downloading" << url << "to" << path;

    QObject::connect(reply, &QNetworkReply::finished, this, [this, path, &loop, &ret]() {
        QNetworkReply *data = (QNetworkReply *) sender();
        QFile localFile(path);
        if (!localFile.open(QIODevice::WriteOnly)) {
            qWarning(UpdaterLog) << "Unable to open file" << path << "for writing!";
            loop.quit();
            return;
        }

        localFile.write(data->readAll());
        localFile.close();
        data->deleteLater();
        ret = true;
        loop.quit();
    });

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(60 * 1000);

    loop.exec();
    return ret;
}

bool UpdateThread::verifySignature(const QString &content, const QString &signature)
{
    QProcess gpg;
    QStringList arguments;

    arguments << "--quiet" << "--verify" << signature << content;

    gpg.start("/usr/bin/gpg", arguments);
    if (!gpg.waitForFinished())
        return false;

    return gpg.exitCode() == 0;
}

bool UpdateThread::casync(const QString &caibx, const QString &dest, const QString &seed, const QUrl &store)
{
    QProcess casync;
    QStringList arguments;

    arguments << "extract"
                << caibx
                << "--seed" << seed
                << "--store" << store.toString()
                << dest;

    casync.start("/usr/bin/casync", arguments);
    if (!casync.waitForFinished())
        return false;

    if (casync.exitCode() != 0) {
        qWarning(UpdaterLog) << "casync failed:";
        qWarning(UpdaterLog) << "-----------------------------";
        qWarning(UpdaterLog) << casync.readAllStandardOutput();
        qWarning(UpdaterLog) << casync.readAllStandardError();
        qWarning(UpdaterLog) << "-----------------------------";
        return false;
    }

    return true;
}

void UpdateThread::run()
{
    const AvailableUpdate *update = updater->getAvailableUpdate();
    QString tmp("/tmp/");
    bool success = false;

    QUrl bootimgSigUrl(update->bootimgUrl);
    bootimgSigUrl.setPath(bootimgSigUrl.path() + ".sig");
    QFileInfo bootimgSigInfo(bootimgSigUrl.path());
    QString bootimgSigLocalFile(tmp + bootimgSigInfo.fileName());

    QFileInfo bootimgInfo(update->bootimgUrl.path());
    QString bootimgLocalFile(tmp + bootimgInfo.fileName());

    QUrl rootfsSigUrl(update->rootfsUrl);
    rootfsSigUrl.setPath(rootfsSigUrl.path() + ".sig");
    QFileInfo rootfsSigInfo(rootfsSigUrl.path());
    QString rootfsSigLocalFile(tmp + rootfsSigInfo.fileName());

    QFileInfo rootfsInfo(update->rootfsUrl.path());
    QString rootfsLocalFile(tmp + rootfsInfo.fileName());

    if (downloadFile(update->bootimgUrl, bootimgLocalFile) &&
        downloadFile(bootimgSigUrl, bootimgSigLocalFile) &&
        downloadFile(update->rootfsUrl, rootfsLocalFile) &&
        downloadFile(rootfsSigUrl, rootfsSigLocalFile)) {

        if (verifySignature(bootimgLocalFile, bootimgSigLocalFile)) {
            if (casync(bootimgLocalFile, updater->getUpdateTarget(Updater::IMAGE_TYPE_BOOTIMG), updater->getUpdateSeed(Updater::IMAGE_TYPE_BOOTIMG), update->casyncStore) &&
                casync(rootfsLocalFile, updater->getUpdateTarget(Updater::IMAGE_TYPE_ROOTFS), updater->getUpdateSeed(Updater::IMAGE_TYPE_ROOTFS), update->casyncStore)) {
                success = true;
            }
        }
    }

    QFile::remove(bootimgLocalFile);
    QFile::remove(bootimgSigLocalFile);
    QFile::remove(rootfsLocalFile);
    QFile::remove(rootfsSigLocalFile);

    if (success)
        emit succeeded();
    else
        emit failed();
}
