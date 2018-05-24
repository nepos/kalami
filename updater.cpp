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

#include <math.h>

#include "updater.h"

Q_LOGGING_CATEGORY(UpdaterLog, "Updater")

Updater::Updater(const Machine *machine, QObject *parent) :
    QObject(parent), machine(machine), networkAccessManager(this)
{
    pendingReply = NULL;
    thread = NULL;
    state = Updater::StateUndefined;
    installedUpdateVersion = QString();
    availableUpdate.version = QString();
}

Updater::~Updater()
{
    if (pendingReply)
        pendingReply->abort();
}

const QString &Updater::getUpdateSeed(Updater::ImageType type) const
{
    switch (type) {
    case Updater::BootImageType:
        return machine->getCurrentBootDevice();

    case Updater::RootfsImageType:
        return machine->getCurrentRootfsDevice();
    }

    return NULL;
}

const QString &Updater::getUpdateTarget(Updater::ImageType type) const
{
    switch (type) {
    case Updater::BootImageType:
        return machine->getAltBootDevice();

    case Updater::RootfsImageType:
        return machine->getAltRootfsDevice();
    }

    return NULL;
}

bool Updater::verifySignature(const QString &contentFile, const QString &signatureFile)
{
    QProcess gpg;
    QStringList arguments;

    arguments << "--quiet" << "--verify" << signatureFile << contentFile;

    gpg.start("/usr/bin/gpg", arguments);
    if (!gpg.waitForFinished())
        return false;

    return gpg.exitCode() == 0;
}

void Updater::downloadFinished()
{
    QNetworkReply *reply = (QNetworkReply *) sender();
    QByteArray content = reply->readAll();

    pendingReply = NULL;
    reply->deleteLater();

    switch (state) {
    case Updater::StateDownloadJson: {
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

        QFile file("/tmp/update.json");
        if (!file.open(QFileDevice::WriteOnly)) {
            qWarning(UpdaterLog) << "Unable to write" << file.fileName();
            break;
        }

        file.write(content);
        file.close();

        if (!doc.isObject() || jsonError.error != QJsonParseError::NoError) {
            emit checkFailed("Unable to parse Json content from update server:" + jsonError.errorString());
            break;
        }

        QString version = machine->getOsVersion();

        QJsonObject json = doc.object();
        availableUpdate.version = json["build_id"].toString();
        availableUpdate.rootfsUrl = QUrl(json["rootfs"].toString());
        availableUpdate.rootfsSha512 = json["rootfs_sha512"].toString();
        availableUpdate.bootimgUrl = QUrl(json["bootimg"].toString());
        availableUpdate.bootimgSha512 = json["bootimg_sha512"].toString();
        availableUpdate.rootfsDeltaUrl = QUrl(json["rootfs_deltas"].toString() + version + ".vcdiff");
        availableUpdate.bootimgDeltaUrl = QUrl(json["bootimg_deltas"].toString() + version + ".vcdiff");

        QNetworkRequest request(QUrl(json["signature"].toString()));
        request.setMaximumRedirectsAllowed(0);

        state = Updater::StateDownloadSignature;
        networkAccessManager.setNetworkAccessible(QNetworkAccessManager::Accessible);
        pendingReply = networkAccessManager.get(request);
        QObject::connect(pendingReply, &QNetworkReply::finished, this, &Updater::downloadFinished);

        break;
    }

    case Updater::StateDownloadSignature: {
        QFile file("/tmp/update.json.sig");
        if (!file.open(QFileDevice::WriteOnly)) {
            qWarning(UpdaterLog) << "Unable to write" << file.fileName();
            break;
        }

        file.write(content);
        file.close();

        state = Updater::StateVerifySignature;

        if (!verifySignature("/tmp/update.json", "/tmp/update.json.sig")) {
            memset(&availableUpdate, 0, sizeof(availableUpdate));
            qWarning() << "Unable to verify signature!";
            break;
        }

        QStringList l = availableUpdate.version.split("-");

        if (l.length() != 2) {
            emit checkFailed("Cannot parse update version from server: " + availableUpdate.version);
            break;
        }

        unsigned long version = l[1].toULong();

        if (installedUpdateVersion == availableUpdate.version) {
            emit alreadyUpToDate();
            break;
        }

        if (l[0] != machine->getOsChannel()) {
            qInfo(UpdaterLog) << "Channel of update" << l[0]
                              << "differs from current device channel" << machine->getOsChannel();
            qInfo(UpdaterLog) << "Forcing update to" << availableUpdate.version;
            emit updateAvailable(availableUpdate.version);
            break;
        }

        if (version > machine->getOsVersionNumber()) {
            emit updateAvailable(availableUpdate.version);
            break;
        }

        emit alreadyUpToDate();

        break;
    }

    default:
        break;
    }
}

void Updater::check(const QString &updateChannel)
{
    QUrlQuery query;
    QString model;

    if (!machine->eligibleForUpdate()) {
        qInfo(UpdaterLog) << "Machine is not eligible for updates!";
        emit updateFailed();
        return;
    }

    switch (machine->getModel()) {
    case Machine::DT410C_EVALBOARD:
    case Machine::NEPOS1:
        model = "nepos1";
        break;
    default:
        model = "unknown";
        break;
    }

    QUrl url("https://os.nepos.io/updates/" + model + "/" + updateChannel + ".json");
    url.setQuery(query);

    qInfo(UpdaterLog) << "Checking for updates on" << url;

    QNetworkRequest request(url);
    request.setRawHeader(QString("X-nepos-current").toLocal8Bit(), machine->getOsVersion().toLocal8Bit());
    request.setRawHeader(QString("X-nepos-machine-id").toLocal8Bit(), machine->getMachineId().toLocal8Bit());
    request.setRawHeader(QString("X-nepos-device-model").toLocal8Bit(), machine->getModelName().toLocal8Bit());
    request.setRawHeader(QString("X-nepos-device-revision").toLocal8Bit(), machine->getDeviceRevision().toLocal8Bit());
    request.setRawHeader(QString("X-nepos-device-serial").toLocal8Bit(), machine->getDeviceSerial().toLocal8Bit());
    request.setMaximumRedirectsAllowed(1);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    if (pendingReply)
        pendingReply->abort();

    state = Updater::StateDownloadJson;
    networkAccessManager.setNetworkAccessible(QNetworkAccessManager::Accessible);
    pendingReply = networkAccessManager.get(request);
    QObject::connect(pendingReply, &QNetworkReply::finished, this, &Updater::downloadFinished);
}

bool Updater::install()
{
    if (availableUpdate.version.isEmpty())
        return false;

    if (thread)
        return false;

    thread = new UpdateThread(this);

    QObject::connect(thread, &UpdateThread::succeeded, [this]() {
        installedUpdateVersion = availableUpdate.version;
        machine->setAltBootConfig();
        emit updateSucceeded();
    });

    QObject::connect(thread, &UpdateThread::failed, [this]() {
        emit updateFailed();
    });

    QObject::connect(thread, &UpdateThread::progress, this, [this](double v) {
        emit updateProgress(v);
    });

    thread->start();

    return true;
}

//
// UpdateThread is a wrapper around a QThread that handles image downloads (both
// VCDIFF delta images and full images) and verifies the written output.
// Its main entry point is an 'AvailableUpdate' where it gets its URLs and SHA512
// sums from. It emits signals for success, failure and progress updates.
//

UpdateThread::UpdateThread(const Updater *updater, QObject *parent) :
    QThread(parent),
    updater(updater),
    lastEmittedProgress(-1)
{
}

void UpdateThread::emitProgress(bool isDownload, double v)
{
    //
    // segment the progress indicator into 4 parts:
    //
    // 25% for boot image download
    // 25% for boot image verification
    // 25% for rootfs download
    // 25% for rootfs verification
    //

    if (v < 0.0f || v > 1.0f)
        return;

    double p = 0.0f;

    switch (state) {
    case UpdateThread::DownloadBootimgState:
        break;

    case UpdateThread::DownloadRootfsState:
        p += 0.5;
        break;
    }

    if (!isDownload)
        p += 0.25;

    p += v/4;
    p = round(p * 100) / 100;

    if (p != lastEmittedProgress)
        emit progress(p);

    lastEmittedProgress = p;
}

bool UpdateThread::downloadDeltaImage(ImageReader::ImageType type,
                                      const QUrl &deltaUrl,
                                      const QString &dictionaryPath,
                                      const QString &outputPath)
{
    QEventLoop loop;
    QTimer timer;
    bool ret = false;
    bool error = false;

    qInfo(UpdaterLog) << "Downloading delta update from" << deltaUrl;

    QNetworkAccessManager networkAccessManager;
    networkAccessManager.setNetworkAccessible(QNetworkAccessManager::Accessible);

    QNetworkRequest request(deltaUrl);
    request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
    request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);

    QNetworkReply *reply = networkAccessManager.get(request);
    reply->setReadBufferSize(1024 * 1024);

    // We need to move these objects to the thread we're running in. Otherwise, the handler for the reply signals
    // will fire in the main thread, leading to memory corruption in reply->readAll()
    networkAccessManager.moveToThread(thread());
    reply->moveToThread(thread());

    BlockDevice output(outputPath);
    if (!output.open(QFile::ReadWrite))
        return false;

    ImageReader dict(type, dictionaryPath);
    if (!dict.open())
        return false;

    const char *buf = (const char *) dict.map();
    if (buf == nullptr)
        return false;

    open_vcdiff::VCDiffStreamingDecoder decoder;
    decoder.SetMaximumTargetFileSize(output.maxSize());
    decoder.StartDecoding(buf, dict.size(), output.map(), output.maxSize());

    QObject::connect(reply, &QNetworkReply::readyRead, [this, &loop, &decoder, &error, &reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qInfo(UpdaterLog) << "Error downloading file: " << reply->errorString();
            reply->abort();
            error = true;
            return;
        }

        const QByteArray data = reply->readAll();
        if (!decoder.DecodeChunkToInterface(data.constData(), data.size())) {
            reply->abort();
            error = true;
            loop.quit();
        }
    });

    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
          [this, &loop, &error, &reply](QNetworkReply::NetworkError code){
        Q_UNUSED(code);

        qInfo(UpdaterLog) << "Error downloading" << reply->url() << ":" << reply->errorString();
        reply->abort();
        error = true;
        loop.quit();
    });

    QObject::connect(reply, &QNetworkReply::finished, [this, &loop, &decoder, &ret, &error]() {
        if (!error)
            decoder.FinishDecoding();

        ret = true;
        loop.quit();
    });

    QObject::connect(reply, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) {
        emitProgress(true, (float) bytesReceived / (float) bytesTotal);
    });

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start(5 * 60 * 1000);
    loop.exec();

    return ret && !error;
}

bool UpdateThread::downloadFullImage(const QUrl &url, const QString &outputPath)
{
    QEventLoop loop;
    QTimer timer;
    bool ret = false;

    QNetworkAccessManager networkAccessManager;
    networkAccessManager.setNetworkAccessible(QNetworkAccessManager::Accessible);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
    request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);

    QNetworkReply *reply = networkAccessManager.get(request);
    reply->setReadBufferSize(1024 * 1024);

    // We need to move these objects to the thread we're running in. Otherwise, the handler for the reply signals
    // will fire in the main thread, leading to memory corruption in reply->readAll()
    networkAccessManager.moveToThread(thread());
    reply->moveToThread(thread());

    BlockDevice output(outputPath);
    if (!output.open(QFile::ReadWrite))
        return false;

    qInfo(UpdaterLog) << "Downloading full image from" << url;

    QObject::connect(reply, &QNetworkReply::readyRead, [this, &output, &reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qInfo(UpdaterLog) << "Error downloading file: " << reply->error();
            reply->abort();
            return;
        }

        const QByteArray data = reply->readAll();
        output.write(data.constData(), data.size());
    });

    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
          [this, &loop, &reply](QNetworkReply::NetworkError code){
        Q_UNUSED(code);

        qInfo(UpdaterLog) << "Error downloading" << reply->url() << ":" << reply->errorString();
        reply->abort();
        loop.quit();
    });

    QObject::connect(reply, &QNetworkReply::finished, [this, &loop, &ret]() {
        ret = true;
        loop.quit();
    });

    QObject::connect(reply, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) {
        emitProgress(true, (float) bytesReceived / (float) bytesTotal);
    });

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start(10 * 60 * 1000);

    loop.exec();

    return ret;
}

bool UpdateThread::verifyImage(ImageReader::ImageType type, const QString &path, const QString &sha512)
{
    QCryptographicHash hash(QCryptographicHash::Sha512);

    ImageReader image(type, path);
    if (!image.open())
        return false;

    qint64 pos = 0;
    const char *buf = (const char *) image.map();
    if (!buf)
        return false;

    while (pos < image.size()) {
        qint64 l = qMin((qint64) 10 * 1024 * 1024, (qint64) (image.size() - pos));
        hash.addData(buf, l);
        pos += l;
        buf += l;

        emitProgress(false, (float) pos / (float) image.size());
    }

    return hash.result().toHex() == sha512;
}

bool UpdateThread::downloadAndVerify(ImageReader::ImageType type,
                                     const QString &dictionaryPath,
                                     const QString &outputPath,
                                     const QUrl &fullImageUrl,
                                     const QUrl &deltaImageUrl,
                                     const QString &sha512)
{
    qInfo(UpdaterLog) << "Installing update to" << outputPath
                      << "using" << dictionaryPath << "as update seed";

    if (downloadDeltaImage(type, deltaImageUrl, dictionaryPath, outputPath) &&
        verifyImage(type, outputPath, sha512))
        return true;

    // Downloading the delta didn't succeed, so let's try the full file
    if (downloadFullImage(fullImageUrl, outputPath) &&
        verifyImage(type, outputPath, sha512))
        return true;

    // Everything failed. We're bricked.
    qInfo(UpdaterLog) << "Full image update failed as well.";

    return false;
}

void UpdateThread::run()
{
    const AvailableUpdate *update = updater->getAvailableUpdate();
    bool ret;

    state = UpdateThread::DownloadBootimgState;
    ret = downloadAndVerify(ImageReader::AndroidBootType,
                            updater->getUpdateSeed(Updater::BootImageType),
                            updater->getUpdateTarget(Updater::BootImageType),
                            update->bootimgUrl, update->bootimgDeltaUrl,
                            update->bootimgSha512);
    if (!ret) {
        emit failed();
        return;
    }

    state = UpdateThread::DownloadRootfsState;
    ret = downloadAndVerify(ImageReader::SquashFsType,
                            updater->getUpdateSeed(Updater::RootfsImageType),
                            updater->getUpdateTarget(Updater::RootfsImageType),
                            update->rootfsUrl, update->rootfsDeltaUrl,
                            update->rootfsSha512);
    if (!ret) {
        emit failed();
        return;
    }

    emit succeeded();
}
