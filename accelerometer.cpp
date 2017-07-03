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
    InputDevice(path, parent),
    currentOrientation(Orientation::Undefined),
    currentAxes{0}
{
    emit orientationChaned(currentOrientation);
}

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
    // Sensor reports int 1000th of g
    const static int gRef = 981;
    // Accept 5% deviation
    const static int upperLimit = 1.05 * gRef;
    const static int lowerLimit = 0.95 * gRef;

    //TODO: Put correct axes here.
    //      This is all pretty basic. Do some proper testing with the
    //      real device!
    qreal *ptrLayingAxis = &currentAxes[0];
    qreal *ptrStandingAxis = &currentAxes[1];

    // Is layingAxis at ~ 1g and currentOrientation != laying
    if (*ptrLayingAxis <= upperLimit && *ptrLayingAxis >= lowerLimit &&
            currentOrientation != Orientation::Laying) {
        currentOrientation = Orientation::Laying;
        emit orientationChaned(currentOrientation);
    }

    // Is standingAxis at ~ 1g and currentOrientation != standing
    if (*ptrStandingAxis <= upperLimit && *ptrStandingAxis >= lowerLimit &&
            currentOrientation != Orientation::Standing) {
        currentOrientation = Orientation::Standing;
        emit orientationChaned(currentOrientation);
    }
}
