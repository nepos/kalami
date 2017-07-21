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
    unsigned long version;
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
    explicit Updater(const Machine *machine, const QString &updateChannel, QObject *parent = 0);
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
    void updateProgress(float progress);

public slots:
    void check();
    void install();

private slots:
    bool verifySignature(const QString &contentFile, const QString &signatureFile);
    void downloadFinished();

private:
    enum State state;
    const Machine *machine;
    QString updateChannel;
    QNetworkAccessManager networkAccessManager;
    QNetworkReply *pendingReply;
    struct AvailableUpdate availableUpdate;
    UpdateThread *thread;
};

class UpdateWriter : public open_vcdiff::OutputStringInterface {
public:
    explicit UpdateWriter();
    ~UpdateWriter();

    bool open(const QString &path);
    void close();

    virtual UpdateWriter &append(const char* s, size_t n);
    virtual void clear();
    virtual void push_back(char c);
    virtual void ReserveAdditionalBytes(size_t res_arg);
    virtual size_t size() const;

private:
    QFile file;
};

class UpdateThread : public QThread
{
    Q_OBJECT

public:
    UpdateThread(const Updater *updater, QObject *parent = 0);
    void run() Q_DECL_OVERRIDE;

signals:
    void progress(float progress);
    void succeeded();
    void failed();

private:
    const Updater *updater;
    bool downloadDeltaImage(const QUrl &deltaUrl, ImageReader *dict, UpdateWriter *output);
    bool downloadFullImage(const QUrl &source, const QString &path);
    bool verifyImage(ImageReader::ImageType type, const QString &path, const QString &sha512);
    bool downloadAndVerify(ImageReader::ImageType type, const QString &dictionaryPath, const QString &outputPath, const QUrl fullImageUrl, const QUrl deltaImageUrl, const QString &sha512);
};

