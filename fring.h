#ifndef FRING_H
#define FRING_H

#include <QObject>
#include "i2cclient.h"

class Fring : public QObject
{
    Q_OBJECT
public:
    explicit Fring(QObject *parent = 0);   
    bool initialize();

signals:

public slots:

private:
    I2CClient client;
};

#endif // FRING_H
