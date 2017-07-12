#include <QTimerEvent>
#include <QDebug>

#include "ambientlightsensor.h"

Q_LOGGING_CATEGORY(AmbientLightSensorLog, "AmbientLightSensor")

AmbientLightSensor::AmbientLightSensor(QString illuminanceFile, float threshold, QObject *parent) :
    QObject(parent),
    input(illuminanceFile),
    interval(interval),
    threshold(threshold)
{
    luminance = -1.0f;
}

void AmbientLightSensor::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timer.timerId())
        return;

    float current = read();

    if (current <= luminance - threshold ||
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
    QByteArray buf = input.readAll();

    return buf.toFloat();
}
