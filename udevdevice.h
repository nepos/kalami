/***
  Copyright (c) 2016 Nepos GmbH

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
