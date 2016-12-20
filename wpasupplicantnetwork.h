#ifndef WPASUPPLICANTNETWORK_H
#define WPASUPPLICANTNETWORK_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusObjectPath>

class WpaSupplicantNetwork : public QObject
{
    Q_OBJECT
public:
    explicit WpaSupplicantNetwork(const QDBusObjectPath &path, QObject *parent = 0);
    ~WpaSupplicantNetwork();

    bool operator ==(const WpaSupplicantNetwork &other);

signals:

public slots:

private:
    QDBusInterface *dbus;
};

#endif // WPASUPPLICANTNETWORK_H
