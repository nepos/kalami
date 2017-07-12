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

#pragma once

#include <QtCore/QLoggingCategory>
#include <QObject>
#include <QFile>
#include <QBasicTimer>

Q_DECLARE_LOGGING_CATEGORY(AmbientLightSensorLog)

class AmbientLightSensor : public QObject
{
    Q_OBJECT
public:
    explicit AmbientLightSensor(QString illuminanceFile, float threshold = 1.0, QObject *parent = 0);

    void start(int interval = 1000);
    void stop();
    float read();

signals:
    void valueChanged(float value);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    QFile input;
    QBasicTimer timer;
    int interval;
    float threshold;
    float luminance;
};
