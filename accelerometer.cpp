/***
  Copyright (c) 2017 Nepos GmbH

  Authors: Pascal Huerst <pascal.huerst@gmail.com>

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

#include <QtCore/QLoggingCategory>
#include <QtDebug>
#include "accelerometer.h"

Q_LOGGING_CATEGORY(AccelerometerLog, "Accelerometer")

Accelerometer::Accelerometer(const QString &path, QObject *parent) :
    InputDevice(path, parent)
{}

Accelerometer::~Accelerometer()
{}

void Accelerometer::update(int type, int code, int value)
{
    if (type != 3)
        return;

    currentAxes[code] = value;
    evaluate();
}

void Accelerometer::evaluate()
{
    //TODO: Figure out, if we changed, and if so, emit orientationChanged()
    qInfo(AccelerometerLog) << "("
                           << currentAxes[0] << "|"
                           << currentAxes[1] << "|"
                           << currentAxes[2] << ")";

    oldAxes[0] = currentAxes[0];
    oldAxes[1] = currentAxes[1];
    oldAxes[2] = currentAxes[2];
}
