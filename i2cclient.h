#ifndef I2CCLIENT_H
#define I2CCLIENT_H

#include <QObject>
#include <QFile>

class I2CClient : public QObject
{
    Q_OBJECT
public:
    explicit I2CClient(QObject *parent = 0);
    ~I2CClient();

    bool open(int bus, int address);
    bool ping();
    bool transfer(unsigned char *sendBuf, size_t sendSize, unsigned char *receiveBuf, size_t receiveSize);

private:
    int address;
    QFile file;
};

#endif // I2CCLIENT_H
