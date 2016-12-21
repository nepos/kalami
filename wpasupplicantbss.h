#ifndef WPASUPPLICANTBSS_H
#define WPASUPPLICANTBSS_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusObjectPath>

class WpaSupplicantBss : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int signal MEMBER signal NOTIFY signalChanged)
    Q_PROPERTY(int age MEMBER age NOTIFY ageChanged)

public:
    explicit WpaSupplicantBss(const QDBusObjectPath &path, QObject *parent = 0);
    ~WpaSupplicantBss();

    bool operator ==(const WpaSupplicantBss &other);

    QByteArray getSSID() const {
        return ssid;
    }

signals:
    void signalChanged(int signal);
    void ageChanged(int age);

public slots:

private slots:
    void propertiesChanged();

private:
    QDBusObjectPath path;
    QDBusInterface *dbus;

    QByteArray ssid;

    int signal;
    int age;
};

#endif // WPASUPPLICANTBSS_H
