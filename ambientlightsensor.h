#ifndef AMBIENTLIGHTSENSOR_H
#define AMBIENTLIGHTSENSOR_H

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

    float read();
};

#endif // AMBIENTLIGHTSENSOR_H
