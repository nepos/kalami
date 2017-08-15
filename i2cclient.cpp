#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "i2cclient.h"

I2CClient::I2CClient(QObject *parent) : QObject(parent), file(), mutex()
{
}

I2CClient::~I2CClient()
{
    file.close();
}

bool I2CClient::open(int bus, int _address)
{
    file.setFileName(QString("/dev/i2c-%1").arg(bus));
    if (!file.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
            return false;

    if (ioctl(file.handle(), I2C_SLAVE, _address) < 0)
        return false;

    address = _address;

    return true;
}

bool I2CClient::ping()
{
    return transfer(NULL, 0, NULL, 0);
}

bool I2CClient::transfer(uint8_t *sendBuf, size_t sendSize , uint8_t *receiveBuf, size_t receiveSize)
{
    if (!file.isOpen())
        return false;

    uint8_t dummy;

    if (!receiveBuf) {
        receiveBuf = &dummy;
        receiveSize = 0;
    }

    struct i2c_msg msgs[] = {
        {
            .addr = (__u16) address,
            .flags = 0,
            .len = (__u16) sendSize,
            .buf = sendBuf
        },
        {
            .addr = (__u16) address,
            .flags = I2C_M_RD | I2C_M_NOSTART,
            .len = (__u16) receiveSize,
            .buf = receiveBuf
        }
    };

    struct i2c_rdwr_ioctl_data data = {
        .msgs = msgs,
        .nmsgs = 2
    };

    QMutexLocker locker(&mutex);

    return ioctl(file.handle(), I2C_RDWR, &data) >= 0;
}
