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

#include <QTimerEvent>
#include <QDebug>

#include "ambientlightsensor.h"

Q_LOGGING_CATEGORY(AmbientLightSensorLog, "AmbientLightSensor")

AmbientLightSensor::AmbientLightSensor(QString illuminanceFile, float threshold, QObject *parent) :
    QObject(parent),
    input(illuminanceFile),
    threshold(threshold)
{
    luminance = -1.0f;
}

void AmbientLightSensor::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timer.timerId())
        return;

    float current = read();

    if (luminance < 0.0f ||
        current <= luminance - threshold ||
        current >= luminance + threshold)
        emit valueChanged(current);

    luminance = current;
}

void AmbientLightSensor::start(int interval)
{
    timer.start(interval, this);
}

void AmbientLightSensor::stop()
{
    timer.stop();
}

float AmbientLightSensor::read()
{
    if (!input.isOpen()) {
        if (!input.open(QIODevice::ReadOnly)) {
            qWarning(AmbientLightSensorLog) << "Error opening" << input.fileName();
            timer.stop();
            return 0.0f;
        }
    }

    input.reset();
    QString str(input.readAll());

    return str.simplified().toFloat();
}
