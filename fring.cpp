#include "fring.h"

Fring::Fring(QObject *parent) : QObject(parent), client()
{
}

bool Fring::initialize()
{
    if (!client.open(0, 0x42))
        return false;

    return true;
}
