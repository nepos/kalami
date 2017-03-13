#ifndef UPDATER_H
#define UPDATER_H

#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>

#include "machine.h"

struct AvailableUpdate {
    unsigned long version;
    QUrl rootfsUrl;
    QUrl bootimgUrl;
};

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(const Machine *machine, const QString &updateChannel, QObject *parent = 0);
    ~Updater();

signals:
    void updateAvailable(const QString &version);

public slots:
    void check();
    int installAvailableUpdate();

private:
    const Machine *machine;
    QString updateChannel;
    QNetworkAccessManager networkAccessManager;
    QNetworkReply *pendingReply;
    struct AvailableUpdate availableUpdate;
};

#endif // UPDATER_H
