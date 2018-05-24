#pragma once

#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QThread>
#include <QFile>
#include <QtCore/QLoggingCategory>

#include <google/vcdecoder.h>

#include "machine.h"
#include "imagereader.h"

Q_DECLARE_LOGGING_CATEGORY(UpdaterLog)

struct AvailableUpdate {
    QString version;
    QUrl rootfsUrl;
    QString rootfsSha512;
    QUrl bootimgUrl;
    QString bootimgSha512;
    QUrl rootfsDeltaUrl;
    QUrl bootimgDeltaUrl;
};

class UpdateThread;

class Updater : public QObject
{
    Q_OBJECT

    enum State {
        StateUndefined,
        StateDownloadJson,
        StateDownloadSignature,
        StateVerifySignature,
    };

public:
    explicit Updater(const Machine *machine, QObject *parent = 0);
    ~Updater();
    const struct AvailableUpdate *getAvailableUpdate() const { return &availableUpdate; }

    enum ImageType {
        BootImageType,
        RootfsImageType,
    };

    const QString &getUpdateSeed(enum ImageType type) const;
    const QString &getUpdateTarget(enum ImageType type) const;

signals:
    void updateAvailable(const QString &version);
    void alreadyUpToDate();
    void checkFailed(const QString &error);
    void updateSucceeded();
    void updateFailed();
    void updateProgress(double progress);

public slots:
    void check(const QString &updateChannel);
    bool install();

private slots:
    bool verifySignature(const QString &contentFile, const QString &signatureFile);
    void downloadFinished();

private:
    enum State state;
    const Machine *machine;
    QNetworkAccessManager networkAccessManager;
    QNetworkReply *pendingReply;
    struct AvailableUpdate availableUpdate;
    QString installedUpdateVersion;
    UpdateThread *thread;
};

class UpdateThread : public QThread
{
    Q_OBJECT

public:
    UpdateThread(const Updater *updater, QObject *parent = 0);
    void run() Q_DECL_OVERRIDE;

signals:
    void progress(double progress);
    void succeeded();
    void failed();

private:

    enum State {
        DownloadBootimgState,
        DownloadRootfsState,
    };

    enum State state;
    const Updater *updater;
    double lastEmittedProgress;
    void emitProgress(bool isDownload, double v);
    bool downloadDeltaImage(ImageReader::ImageType type, const QUrl &deltaUrl, const QString &dictionaryPath, const QString &outputPath);
    bool downloadFullImage(const QUrl &source, const QString &outputPath);
    bool verifyImage(ImageReader::ImageType type, const QString &path, const QString &sha512);
    bool downloadAndVerify(ImageReader::ImageType type, const QString &dictionaryPath, const QString &outputPath, const QUrl &fullImageUrl, const QUrl &deltaImageUrl, const QString &sha512);
};

