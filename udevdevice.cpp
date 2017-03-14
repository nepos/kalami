/***
  Copyright (c) 2017 Nepos GmbH

  Authors: Daniel Mack <daniel@nepos.io>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
***/

#include "udevdevice.h"

Q_LOGGING_CATEGORY(UDevDeviceLog, "UDevDevice")

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
