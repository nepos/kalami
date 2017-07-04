#ifndef FRING_H
#define FRING_H

#include <QObject>
#include "i2cclient.h"
#include "gpio.h"

class Fring : public QObject
{
    Q_OBJECT
public:
    explicit Fring(QObject *parent = 0);   
    bool initialize();

signals:

public slots:


private slots:
    void onInterrupt(int fd);

private:
    static const int GPIONr;

    I2CClient client;
    GPIO interruptGpio;
};

#endif // FRING_H
