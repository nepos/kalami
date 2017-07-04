#include "gpio.h"

#include <QFile>
#include <QSocketNotifier>

Q_LOGGING_CATEGORY(GPIOLog, "GPIO")

const QString GPIO::basePath = "/sys/class/gpio";

GPIO::GPIO(int number, QObject *parent) :
    QObject(parent),
    gpioPath(GPIO::basePath + "/gpio" + QString::number(number)),
    number(number),
    pathExport(GPIO::basePath + "/export"),
    pathUnexport(GPIO::basePath + "/unexport")
{
    QFile f(pathExport);
    if (!f.open(QFile::WriteOnly)) {
        qWarning(GPIOLog) << "Can not open file: " << pathExport;
        return;
    }
    f.write(QString::number(number).toLocal8Bit());
}

GPIO::~GPIO()
{
    closeValueFile();

    QFile f(pathUnexport);
    if (!f.open(QFile::WriteOnly)) {
        qWarning(GPIOLog) << "Can not open file: " << pathUnexport;
        return;
    }
    f.write(QString::number(number).toLocal8Bit());
}

void GPIO::setDirection(Direction io)
{
    direction = io;
    QFile f(gpioPath + "/direction");
    if (!f.open(QFile::WriteOnly)) {
        qWarning(GPIOLog) << "Can not open file: " << gpioPath << "/direction";
        return;
    }

    if (direction == GPIO::DirectionIn) {
        f.write("in", 2);

        openValueFile(QFile::ReadOnly);

        auto sn = new QSocketNotifier(valueFile.handle(), QSocketNotifier::Read, this);
        connect(sn, SIGNAL(activated(int)), this, SIGNAL(onDataReady(int)));

    } else {
        f.write("out", 3);

        openValueFile(QFile::WriteOnly);
    }
}

void GPIO::set(GPIO::Value v)
{
    if (!valueFile.isOpen()) {
        qWarning(GPIOLog) << "Value file not opened, ignoring write: " << gpioPath << "/value";
        return;
    }

    if (direction != GPIO::DirectionOut) {
        qWarning(GPIOLog) << "Can not write to gpio, it's an input: " << number;
        return;
    }

    char val = static_cast<char>(v);
    valueFile.write(&val, 1);
    valueFile.flush();
}

void GPIO::setEdge(Edge e)
{
    QString data;

    if (e == GPIO::EdgeRising)
        data = "rising";
    else if (e == GPIO::EdgeFalling)
        data = "falling";
    else if (e == GPIO::EdgeBoth)
        data = "both";

    QFile f(gpioPath + "/edge");
    if (!f.open(QFile::WriteOnly)) {
        qWarning(GPIOLog) << "Can not open file: " << gpioPath << "/edge";
        return;
    }

    f.write(data.toLocal8Bit());
}

void GPIO::openValueFile(QIODevice::OpenModeFlag f)
{
    valueFile.setFileName(gpioPath + "/value");
    if (!valueFile.open(f)) {
        qWarning(GPIOLog) << "Can not open file: " << gpioPath << "/value";
        return;
    }
}

void GPIO::closeValueFile()
{
    if (valueFile.isOpen())
        valueFile.close();

}

