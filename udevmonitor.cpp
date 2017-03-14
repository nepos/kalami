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

#include <QDebug>
#include "udevmonitor.h"

Q_LOGGING_CATEGORY(UDevMonitorLog, "UDevMonitor")

UDevMonitor::UDevMonitor(QObject *parent) :
    QObject(parent)
{
    udev = udev_new();
    monitor = udev_monitor_new_from_netlink(udev, "udev");

    int fd = udev_monitor_get_fd(monitor);
    notifier = new QSocketNotifier(fd, QSocketNotifier::Read);

    QObject::connect(notifier, &QSocketNotifier::activated, [this]() {
        struct udev_device *udev_device;

        while ((udev_device = udev_monitor_receive_device(monitor))) {
            UDevDevice *dev = new UDevDevice(udev_device);
            udev_device_unref(udev_device);

            if (dev->getDevNode().isEmpty()) {
                delete dev;
                continue;
            }

            if (dev->getAction() == "add") {
                if (!devices.contains(dev))
                    devices.append(dev);

                emit deviceAdded(*dev);
            }

            if (dev->getAction() == "remove") {
                emit deviceRemoved(*dev);
                devices.removeAll(dev);
                delete dev;
            }
        }
    });

    udev_monitor_enable_receiving(monitor);
}

bool UDevMonitor::addMatchSubsystem(const QString &subsystem)
{
    struct udev_list_entry *matched, *entry;
    struct udev_enumerate *enumerate;
    int r;

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, qPrintable(subsystem));
    udev_enumerate_scan_devices(enumerate);
    matched = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(entry, matched) {
        struct udev_device *udev_device = udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
        UDevDevice *dev = new UDevDevice(udev_device);
        udev_device_unref(udev_device);

        devices.append(dev);
        emit deviceAdded(*dev);
    }

    udev_enumerate_unref(enumerate);

    r = udev_monitor_filter_add_match_subsystem_devtype(monitor, qPrintable(subsystem), NULL);
    if (r < 0)
        return false;

    return true;
}

UDevMonitor::~UDevMonitor()
{
    while (!devices.isEmpty())
        delete devices.takeFirst();

    udev_monitor_unref(monitor);
    udev_unref(udev);
}
