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
#include "brightnesscontrol.h"

Q_LOGGING_CATEGORY(BrightnessControlLog, "BrightnessControl")

BrightnessControl::BrightnessControl(const QString &rootPath, QObject *parent) :
    QObject(parent), brightnessFile()
{
    brightnessFile.setFileName(rootPath + "/brightness");

    QFile max(rootPath + "/max_brightness");
    if (max.open(QIODevice::ReadOnly)) {
        QString value = max.readLine();
        maxBrightness = value.toInt();
        max.close();
    } else
        maxBrightness = 1;
}

BrightnessControl::~BrightnessControl()
{
}

bool BrightnessControl::setBrightness(float value)
{
    if (brightnessFile.exists() &&
        brightnessFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QString str = QString::number((int) (value * (float) maxBrightness));
        qInfo(BrightnessControlLog) << "Setting brightness of" << brightnessFile.fileName()
                                    << "to" << str;
        str += "\n";
        qint64 r = brightnessFile.write(str.toLocal8Bit());
        brightnessFile.close();

        return r == str.toLocal8Bit().length();
    } else {
        qWarning(BrightnessControlLog) << "Unable to open brightness file!";
        return false;
    }
}

float BrightnessControl::getBrightness()
{
    int val = 0;

    if (brightnessFile.open(QIODevice::ReadOnly)) {
        QString value = brightnessFile.readLine();
        val = value.toInt();
        brightnessFile.close();
    }

    return  (float) val / (float) maxBrightness;
}
