#ifndef FRING_H
#define FRING_H

#include <QObject>
#include "i2cclient.h"
#include "gpio.h"

Q_DECLARE_LOGGING_CATEGORY(FringLog)

class Fring : public QObject
{
    Q_OBJECT
public:
    explicit Fring(QObject *parent = 0);   
    bool initialize();

signals:

public slots:


private slots:
    void onInterrupt(GPIO::Value v);

private:
    static const int GPIONr;
    static const int I2CAddr;

    I2CClient client;
    GPIO interruptGpio;
};

#endif // FRING_H
