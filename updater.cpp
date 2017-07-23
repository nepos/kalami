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
    state = Updater::StateUndefined;
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

    switch (state) {
    case Updater::StateDownloadJson: {
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

        QFile file("/tmp/update.json");
        if (!file.open(QFileDevice::WriteOnly))
            return;

        file.write(content);
        file.close();

        if (jsonError.error != QJsonParseError::NoError) {
            emit checkFailed("Unable to parse Json content from update server:" + jsonError.errorString());
            return;
        }

        QString version = QString::number(machine->getOsVersion());

        QJsonObject json = doc.object();
        availableUpdate.version = json["build_id"].toString().toULong();
        availableUpdate.rootfsUrl = QUrl(json["rootfs"].toString());
        availableUpdate.rootfsSha512 = json["rootfs_sha512"].toString();
        availableUpdate.bootimgUrl = QUrl(json["bootimg"].toString());
        availableUpdate.bootimgSha512 = json["bootimg_sha512"].toString();
        availableUpdate.rootfsDeltaUrl = QUrl(json["rootfs_deltas"].toString() + version + ".vcdiff");
        availableUpdate.bootimgDeltaUrl = QUrl(json["bootimg_deltas"].toString() + version + ".vcdiff");

        QNetworkRequest request(QUrl(json["signature"].toString()));
        state = Updater::StateDownloadSignature;
        pendingReply = networkAccessManager.get(request);
        QObject::connect(pendingReply, &QNetworkReply::finished, this, &Updater::downloadFinished);

        break;
    }

    case Updater::StateDownloadSignature: {
        QFile file("/tmp/update.json.sig");
        if (!file.open(QFileDevice::WriteOnly))
            return;

        file.write(content);
        file.close();

        state = Updater::StateVerifySignature;

        if (!verifySignature("/tmp/update.json", "/tmp/update.json.sig")) {
            memset(&availableUpdate, 0, sizeof(availableUpdate));
            qWarning() << "Unable to verify signature!";
            break;
        }

        if (availableUpdate.version > machine->getOsVersion())
            emit updateAvailable(QString::number(availableUpdate.version));
        else
            emit alreadyUpToDate();

        break;
    }
    default:
        break;
    }

    reply->deleteLater();
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

    if (pendingReply) {
        pendingReply->abort();
        pendingReply->deleteLater();
    }

    state = Updater::StateDownloadJson;
    pendingReply = networkAccessManager.get(request);
    QObject::connect(pendingReply, &QNetworkReply::finished, this, &Updater::downloadFinished);
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

    QObject::connect(thread, &UpdateThread::progress, this, [this](float v) {
        emit updateProgress(v);
    });

    thread->start();
}



UpdateWriter::UpdateWriter() : file()
{
}

UpdateWriter::~UpdateWriter()
{
    if (file.isOpen())
        file.close();
}

bool UpdateWriter::open(const QString &path)
{
    file.setFileName(path);
    return file.open(QFileDevice::WriteOnly);
}

void UpdateWriter::close()
{
    file.close();
}

UpdateWriter &UpdateWriter::append(const char *s, size_t n)
{
    file.write(s, n);
    return *this;
}

void UpdateWriter::clear()
{
    file.reset();
}

void UpdateWriter::push_back(char c)
{
    file.write(&c, 1);
}

void UpdateWriter::ReserveAdditionalBytes(size_t res_arg)
{
    file.resize(file.pos() + res_arg);
}

size_t UpdateWriter::size() const
{
    return file.pos();
}

UpdateThread::UpdateThread(const Updater *updater, QObject *parent) :
    QThread(parent),
    updater(updater)
{
}

bool UpdateThread::downloadDeltaImage(const QUrl &deltaUrl, ImageReader *dict, UpdateWriter *output)
{
    QNetworkAccessManager networkAccessManager;
    QNetworkRequest request(deltaUrl);
    QNetworkReply *reply = networkAccessManager.get(request);
    QEventLoop loop;
    QTimer timer;
    bool ret = false;
    bool error = false;

    networkAccessManager.moveToThread(thread());
    reply->moveToThread(thread());

    open_vcdiff::VCDiffStreamingDecoder decoder;
    decoder.SetMaximumTargetFileSize(512 * 1024 * 1024);
    decoder.StartDecoding((const char *) dict->map(), dict->size());

    QObject::connect(reply, &QNetworkReply::readyRead, this, [this, &loop, &decoder, output, &error]() {
        QNetworkReply *reply = (QNetworkReply *) sender();

        if (reply->error() != QNetworkReply::NoError) {
            qInfo() << "Error downloading file: " << reply->error();
            reply->abort();
            error = true;
            return;
        }

        const QByteArray data = reply->readAll();

        if (!decoder.DecodeChunkToInterface(data.constData(), data.size(), output)) {
            reply->abort();
            error = true;
            loop.quit();
        }
    });

    QObject::connect(reply, &QNetworkReply::finished, this, [this, &loop, &decoder, &ret]() {
        decoder.FinishDecoding();
        ret = true;
        loop.quit();
    });

    QObject::connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal) {
        float v = (float) bytesReceived / (float) bytesTotal;
        // reserve 50% for image download, 50% for verification
        emit progress(v/2);
    });

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start(60 * 1000);
    loop.exec();

    reply->abort();
    reply->deleteLater();

    return ret && !error;
}

bool UpdateThread::downloadFullImage(const QUrl &url, UpdateWriter *output)
{
    QNetworkAccessManager networkAccessManager;
    QNetworkRequest request(url);
    QNetworkReply *reply = networkAccessManager.get(request);
    QEventLoop loop;
    QTimer timer;
    bool ret = false;

    networkAccessManager.moveToThread(thread());
    reply->moveToThread(thread());

    QObject::connect(reply, &QNetworkReply::readyRead, this, [this, output]() {
        QNetworkReply *reply = (QNetworkReply *) sender();

        if (reply->error() != QNetworkReply::NoError) {
            qInfo(UpdaterLog) << "Error downloading file: " << reply->error();
            reply->abort();
            return;
        }

        const QByteArray data = reply->readAll();
        output->append(data.constData(), data.size());
    });

    QObject::connect(reply, &QNetworkReply::finished, this, [this, &loop, &ret]() {
        ret = true;
        loop.quit();
    });

    QObject::connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal) {
        float v = (float) bytesReceived / (float) bytesTotal;
        // reserve 50% for image download, 50% for verification
        emit progress(v/2);
    });

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start(60 * 1000);

    loop.exec();

    reply->abort();
    reply->deleteLater();

    return ret;
}

bool UpdateThread::verifyImage(ImageReader::ImageType type, const QString &path, const QString &sha512)
{
    QCryptographicHash hash(QCryptographicHash::Sha512);

    ImageReader image(type, path);
    if (!image.open())
        return false;

    qint64 pos = 0;
    char *buf = (char *) image.map();

    while (pos < image.size()) {
        qint64 l = qMin((qint64) 1024 * 1024, (qint64) (image.size() - pos));
        hash.addData(buf, l);
        pos += l;
        buf += l;

        emit progress(0.5f + ((float) pos / (float) image.size()) / 2);
    }

    return hash.result().toHex() == sha512;
}

bool UpdateThread::downloadAndVerify(ImageReader::ImageType type,
                                     const QString &dictionaryPath,
                                     const QString &outputPath,
                                     const QUrl fullImageUrl,
                                     const QUrl deltaImageUrl,
                                     const QString &sha512)
{
    UpdateWriter output;
    if (!output.open(outputPath))
        return false;

    ImageReader dict(type, dictionaryPath);
    if (dict.open()) {
        if (downloadDeltaImage(deltaImageUrl, &dict, &output) && verifyImage(type, outputPath, sha512))
            return true;

        dict.close();
    }

    qInfo(UpdaterLog) << "Downloading delta update from" << deltaImageUrl << "failed";
    qInfo(UpdaterLog) << "Trying full image from" << fullImageUrl;

    // Downloading the delta didn't succeed, so let's try the full file
    if (downloadFullImage(fullImageUrl, &output) && verifyImage(type, outputPath, sha512))
        return true;

    // Everything failed. We're bricked.
    qInfo(UpdaterLog) << "Full image update failed as well.";

    return false;
}

void UpdateThread::run()
{
    const AvailableUpdate *update = updater->getAvailableUpdate();
    bool ret;

    ret = downloadAndVerify(ImageReader::AndroidBootType,
                            updater->getUpdateSeed(Updater::BootImageType),
                            updater->getUpdateTarget(Updater::BootImageType),
                            update->bootimgUrl, update->bootimgDeltaUrl,
                            update->bootimgSha512);
    if (!ret) {
        emit failed();
        return;
    }

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
