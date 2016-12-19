#include "udevdevice.h"

UDevDevice::UDevDevice(struct udev_device *udev_device, QObject *parent) :
    QObject(parent)
{
    dev = udev_device_ref(udev_device);
}

const QString UDevDevice::getDevPath() const
{
    return QString(qPrintable(udev_device_get_devpath(dev)));
}

const QString UDevDevice::getDevSubsystem() const
{
    return QString(qPrintable(udev_device_get_subsystem(dev)));
}

const QString UDevDevice::getDevType() const
{
    return QString(qPrintable(udev_device_get_devtype(dev)));
}

const QString UDevDevice::getDevNode() const
{
    return QString(qPrintable(udev_device_get_devnode(dev)));
}

const QString UDevDevice::getSysPath() const
{
    return QString(qPrintable(udev_device_get_syspath(dev)));
}

const QString UDevDevice::getSysName() const
{
    return QString(qPrintable(udev_device_get_sysname(dev)));
}

const QString UDevDevice::getSysNum() const
{
    return QString(qPrintable(udev_device_get_sysnum(dev)));
}

const QString UDevDevice::getDriver() const
{
    return QString(qPrintable(udev_device_get_driver(dev)));
}

const QString UDevDevice::getAction() const
{
    return QString(qPrintable(udev_device_get_action(dev)));
}

const QString UDevDevice::getSysAttrValue(const QString &attr) const
{
    return QString(qPrintable(udev_device_get_sysattr_value(dev, qPrintable(attr))));
}

bool UDevDevice::operator == (const UDevDevice &other)
{
    return getSysPath() == other.getSysPath();
}

UDevDevice::~UDevDevice()
{
    udev_device_unref(dev);
}
