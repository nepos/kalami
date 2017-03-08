#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QNetworkAccessManager>

#include "machine.h"

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(const Machine *machine, const QString &updateChannel, QObject *parent = 0);

signals:
    void updateAvailable(const QString &version);

public slots:
    void check();

private:
    const Machine *machine;
    QString updateChannel;
    QNetworkAccessManager networkAccessManager;
};

#endif // UPDATER_H
