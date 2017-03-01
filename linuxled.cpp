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
#include "linuxled.h"

LinuxLED::LinuxLED(const QString &name, QObject *parent) :
    QObject(parent), brightnessFile()
{
    QString dir = "/sys/class/leds/" + name;

    brightnessFile.setFileName(dir + "/brightness");
    brightnessFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered);

    QFile max(dir + "/max_brightness");
    if (max.open(QIODevice::ReadOnly)) {
        QString value = max.readLine();
        maxBrightness = value.toInt();
        max.close();
    } else
        maxBrightness = 1;
}

LinuxLED::~LinuxLED()
{
    brightnessFile.close();
}

void LinuxLED::setBrightness(float value)
{
    if (brightnessFile.isOpen()) {
        QString str = QString::number(value * (float) maxBrightness);
        brightnessFile.write(str.toLocal8Bit() + "\n");
    }
}
