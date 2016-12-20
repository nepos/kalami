#ifndef WPASUPPLICANT_H
#define WPASUPPLICANT_H

#include <QObject>
#include <QDBusInterface>

#include <wpasupplicantinterface.h>

class WpaSupplicant : public QObject
{
    Q_OBJECT
public:
    explicit WpaSupplicant(QObject *parent = 0);
    ~WpaSupplicant();

    void start();

signals:
    void interfaceAdded(const WpaSupplicantInterface &interface);

public slots:

private:
    QDBusInterface *dbus;
    QList<WpaSupplicantInterface *> interfaces;
};

#endif // WPASUPPLICANT_H
