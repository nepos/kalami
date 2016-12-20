#ifndef WPASUPPLICANTINTERFACE_H
#define WPASUPPLICANTINTERFACE_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusObjectPath>

#include "wpasupplicantbss.h"
#include "wpasupplicantnetwork.h"

class WpaSupplicantInterface : public QObject
{
    Q_OBJECT
public:
    explicit WpaSupplicantInterface(const QDBusObjectPath &path, QObject *parent = 0);
    ~WpaSupplicantInterface();

    void start();
    const QString getIfname() const;

    bool operator ==(const WpaSupplicantInterface &other);

signals:

public slots:

private:
    QDBusInterface *dbus;

    QList<WpaSupplicantNetwork *> networks;
    QList<WpaSupplicantBss *> bsss;
};

#endif // WPASUPPLICANTINTERFACE_H
