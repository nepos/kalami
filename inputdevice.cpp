/***
  Copyright (c) 2016 Nepos GmbH

  Authors: Daniel Mack <daniel@nepos.io>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this software; If not, see <http://www.gnu.org/licenses/>.
***/

#include <QDebug>

#include <linux/input.h>
#include "inputdevice.h"

#define BITS_PER_LONG   (sizeof(long) * 8)
#define NBITS(x)        ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)          ((x)%BITS_PER_LONG)
#define BIT(x)          (1UL<<OFF(x))
#define LONG(x)         ((x)/BITS_PER_LONG)
#define test_bit(bit, array) ((array[LONG(bit)] >> OFF(bit)) & 1)

InputDevice::InputDevice(const QString &path, QObject *parent) :
    QObject(parent), device(path)
{
    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered)) {
        qWarning() << "Unable to open file " << path << ":" << device.errorString();
        return;
    }

    QObject::connect(&device, &QIODevice::readyRead, [this]() {
        struct input_event ev;
        qint64 r;

        r = device.read((char *) &ev, (qint64) sizeof(ev));
        if (r == sizeof(ev))
            emit inputEvent(ev.type, ev.code, ev.value);
        else
            qWarning() << "Short read from device " << device.fileName();
    });
}

void InputDevice::emitCurrent()
{
    unsigned long bit[EV_MAX][NBITS(KEY_MAX)] = { 0 };
    unsigned long state[KEY_CNT] = { 0 };
    unsigned int type, code;
    int ret;

    if (!device.isOpen())
        return;

    int fd = device.handle();

    ret = ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
    if (ret < 0) {
        qWarning() << "ioctl(EVIOCGBIT) failed for " << device.fileName() << ":" << device.errorString();
        return;
    }

    for (type = 0; type < EV_MAX; type++) {
        if (test_bit(type, bit[0]) && type != EV_REP) {
            if (type == EV_KEY) {
                ioctl(fd, EVIOCGBIT(type, KEY_MAX), bit[EV_KEY]);

                for (code = 0; code < KEY_MAX; code++) {
                    if (test_bit(code, bit[type])) {
                        int value = test_bit(code, state);

                        emit inputEvent(type, code, value);
                    }
                }
            }
        }
    }
}

InputDevice::~InputDevice()
{
    if (device.isOpen())
        device.close();
}
