#include "fring.h"
#include "gpio.h"

const int Fring::GPIONr = 8;

Fring::Fring(QObject *parent) :
    QObject(parent),
    client(this),
    interruptGpio(Fring::GPIONr, this)
{
}

bool Fring::initialize()
{
    if (!client.open(0, 0x42))
        return false;

    return true;
}


void Fring::onInterrupt(int fd)
{



}
