#ifndef UDEVDEVICE_H
#define UDEVDEVICE_H

#include <QObject>
#include <libudev.h>

class UDevDevice : public QObject
{
    Q_OBJECT

public:
    explicit UDevDevice(struct udev_device *udev_device, QObject *parent = 0);
    ~UDevDevice();

    const QString getDevPath() const;
    const QString getDevSubsystem() const;
    const QString getDevType() const;
    const QString getDevNode() const;
    const QString getSysPath() const;
    const QString getSysName() const;
    const QString getSysNum() const;
    const QString getDriver() const;
    const QString getAction() const;
    const QString getSysAttrValue(const QString &attr) const;

    bool operator ==(const UDevDevice &other);

private:
    struct udev_device *dev;
};

#endif // UDEVDEVICE_H
