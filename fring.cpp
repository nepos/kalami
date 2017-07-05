#include "fring.h"
#include "gpio.h"

Q_LOGGING_CATEGORY(FringLog, "Fring")

const int Fring::GPIONr = 8;
const int Fring::I2CAddr = 0x42;

Fring::Fring(QObject *parent) :
    QObject(parent),
    client(this),
    interruptGpio(Fring::GPIONr, this)
{
    interruptGpio.setEdge(GPIO::EdgeFalling);
    interruptGpio.setDirection(GPIO::DirectionIn);
    QObject::connect(&interruptGpio, &GPIO::onDataReady, this, &Fring::onInterrupt);
}

bool Fring::initialize()
{
    if (!client.open(0, Fring::I2CAddr))
        return false;

    return true;
}


void Fring::onInterrupt(GPIO::Value v)
{
    // Read fring over i2c here!

}
