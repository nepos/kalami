#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "i2cclient.h"

Q_LOGGING_CATEGORY(I2CClientLog, "I2CClientLog")

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

    if (!file.exists())
        return false;

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

bool I2CClient::isOpen() const
{
    return file.isOpen();
}

bool I2CClient::transfer(uint8_t *sendBuf, size_t sendSize , uint8_t *receiveBuf, size_t receiveSize)
{
    if (!file.isOpen())
        return false;

    struct i2c_msg msgs[2] = {
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

    if (!receiveBuf || receiveSize == 0)
        data.nmsgs = 1;

    QMutexLocker locker(&mutex);

    int ret = ioctl(file.handle(), I2C_RDWR, &data);

    if (ret < 0) {
        QString str;
        str.sprintf("I2C client transfer (%02X  %lu out, %lu in. %d messages) failed: %s", sendBuf[0],  sendSize, receiveSize, data.nmsgs, strerror(errno));
        qWarning(I2CClientLog) << str;
        return false;
    }

    return true;
}
