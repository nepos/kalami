#ifndef UDEVCLIENT_H
#define UDEVCLIENT_H

#include <QObject>
#include <QSocketNotifier>

#include <libudev.h>
#include "udevdevice.h"

class UDevMonitor : public QObject
{
    Q_OBJECT
public:
    explicit UDevMonitor(QObject *parent = 0);
    ~UDevMonitor();

    bool addMatchSubsystem(const QString &subsystem);

signals:
    void deviceAdded(const UDevDevice &UDevMonitor);
    void deviceRemoved(const UDevDevice &UDevMonitor);

private:
    struct udev *udev;
    struct udev_monitor *monitor;

    QSocketNotifier *notifier;

    QList<UDevDevice *> devices;
};

#endif // UDEVCLIENT_H
