#ifndef WPASUPPLICANTBSS_H
#define WPASUPPLICANTBSS_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusObjectPath>

class WpaSupplicantBss : public QObject
{
    Q_OBJECT
public:
    explicit WpaSupplicantBss(const QDBusObjectPath &path, QObject *parent = 0);
    ~WpaSupplicantBss();

    bool operator ==(const WpaSupplicantBss &other);

signals:

public slots:

private:
    QDBusObjectPath path;
    QDBusInterface *dbus;
};

#endif // WPASUPPLICANTBSS_H
