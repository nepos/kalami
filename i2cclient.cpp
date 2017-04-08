#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "i2cclient.h"

I2CClient::I2CClient(QObject *parent) : QObject(parent), file()
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

bool I2CClient::transfer(unsigned char *sendBuf, size_t sendSize , unsigned char *receiveBuf, size_t receiveSize)
{
    if (!file.isOpen())
        return false;

    struct i2c_msg msgs[] = {
        {
            .addr = (__u16) address,
            .flags = I2C_M_RD,
            .len = (__u16) sendSize,
            .buf = sendBuf
        },
        {
            .addr = (__u16) address,
            .flags = I2C_M_STOP,
            .len = (__u16) receiveSize,
            .buf = receiveBuf
        }
    };

    struct i2c_rdwr_ioctl_data data = {
        .msgs = msgs,
        .nmsgs = 1
    };

    if (sendSize == 0) {
        msgs[0].flags |= I2C_M_STOP;
        data.nmsgs = 1;
    }

    return ioctl(file.handle(), I2C_RDWR, &data) == 0;
}
