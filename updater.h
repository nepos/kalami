#ifndef UPDATER_H
#define UPDATER_H

#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QThread>
#include <QtCore/QLoggingCategory>

#include "machine.h"

Q_DECLARE_LOGGING_CATEGORY(UpdaterLog)

struct AvailableUpdate {
    unsigned long version;
    QUrl rootfsUrl;
    QUrl bootimgUrl;
    QUrl casyncStore;
};

class Updater;

class UpdateThread : public QThread
{
    Q_OBJECT

public:
    UpdateThread(const Updater *updater, QObject *parent = 0);
    void run() Q_DECL_OVERRIDE;

signals:
    void succeeded();
    void failed();

private:
    const Updater *updater;
    bool downloadFile(const QUrl &source, const QString &path);
    bool verifySignature(const QString &content, const QString &signature);
    bool casync(const QString &caibx, const QString &dest, const QString &seed, const QUrl &store);
};

class Updater : public QObject
{
    Q_OBJECT

public:
    explicit Updater(const Machine *machine, const QString &updateChannel, QObject *parent = 0);
    ~Updater();
    const struct AvailableUpdate *getAvailableUpdate() const { return &availableUpdate; }

    enum ImageType {
        IMAGE_TYPE_BOOTIMG,
        IMAGE_TYPE_ROOTFS,
    };

    const QString &getUpdateSeed(enum ImageType type) const;
    const QString &getUpdateTarget(enum ImageType type) const;

signals:
    void updateAvailable(const QString &version);
    void alreadyUpToDate();
    void checkFailed(const QString &error);
    void updateSucceeded();
    void updateFailed();

public slots:
    void check();
    void install();

private:
    const Machine *machine;
    QString updateChannel;
    QNetworkAccessManager networkAccessManager;
    QNetworkReply *pendingReply;
    struct AvailableUpdate availableUpdate;
    UpdateThread *thread;
};

#endif // UPDATER_H
