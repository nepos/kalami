#include "gpio.h"

#include <QFile>
#include <QSocketNotifier>
#include <QFile>

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
    if (!f.exists() || !f.open(QFile::WriteOnly)) {
        qWarning(GPIOLog) << "Can not open file: " << pathExport;
        return;
    }
    f.write(QString::number(number).toLocal8Bit());
    f.flush();
}

GPIO::~GPIO()
{
    closeValueFile();

    QFile f(pathUnexport);
    if (!f.exists() || !f.open(QFile::WriteOnly)) {
        qWarning(GPIOLog) << "Can not open file: " << pathUnexport;
        return;
    }
    f.write(QString::number(number).toLocal8Bit());
    f.flush();
}

void GPIO::setDirection(Direction io)
{
    direction = io;
    QFile f(gpioPath + "/direction");
    if (!f.exists() || !f.open(QFile::WriteOnly)) {
        qWarning(GPIOLog) << "Can not open file: " << gpioPath << "/direction";
        return;
    }

    if (direction == GPIO::DirectionIn) {
        f.write("in", 2);
        f.flush();

        openValueFile(QFile::ReadOnly);

        auto sn = new QSocketNotifier(valueFile.handle(), QSocketNotifier::Exception, this);
        QObject::connect(sn, &QSocketNotifier::activated, [this](int fd) {
            Q_UNUSED(fd);
            valueFile.reset();
            auto buf = valueFile.readAll();
            if (buf.isEmpty()) {
                qWarning(GPIOLog) << "Can not read GPIO value. Buffer is empty.";
                return;
            }

            GPIO::Value v = buf.at(0) ? ValueHi : ValueLo;
            emit onDataReady(v);
        });
    } else {
        f.write("out", 3);
        f.flush();

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
    f.flush();
}

void GPIO::openValueFile(QIODevice::OpenModeFlag f)
{
    valueFile.setFileName(gpioPath + "/value");
    if (!valueFile.exists() || !valueFile.open(f)) {
        qWarning(GPIOLog) << "Can not open file: " << gpioPath << "/value";
        return;
    }
}

void GPIO::closeValueFile()
{
    if (valueFile.isOpen())
        valueFile.close();

}

