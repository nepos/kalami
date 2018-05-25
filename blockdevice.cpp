#include <QDebug>

#include <linux/fs.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "blockdevice.h"

Q_LOGGING_CATEGORY(BlockDeviceLog, "BlockDevice")

BlockDevice::BlockDevice(const QString path, QObject *parent) :
    QObject(parent),
    file(path),
    deviceMaxSize(0),
    mappedBuffer(NULL)
{
}

BlockDevice::~BlockDevice()
{
    close();
}

bool BlockDevice::open(QFile::OpenMode m)
{
    int ret;
    struct stat stat;

    if (!file.open(m)) {
        qWarning(BlockDeviceLog) << "Error opening" << file.fileName()
                                 << ":" << file.errorString();
        return false;
    }

    ret = fstat(file.handle(), &stat);
    if (ret < 0)
        return false;

    if (!S_ISBLK(stat.st_mode)) {
        qWarning(BlockDeviceLog) << file.fileName() << "is not a block device";
        return false;
    }

    ret = ioctl(file.handle(), BLKGETSIZE64, &deviceMaxSize);
    if (ret < 0) {
        qWarning() << "Cannot determine size of device" << file.fileName();
        return false;
    }

    return true;
}

void BlockDevice::close()
{
    if (mappedBuffer) {
        munmap(mappedBuffer, deviceMaxSize);
        mappedBuffer = NULL;
    }

    file.close();
}

char *BlockDevice::map()
{
    if (!file.isOpen())
        return NULL;

    if (!mappedBuffer) {
        int prot = 0;

        if (file.openMode() & QFile::ReadOnly)
            prot |= PROT_READ;

        if (file.openMode() & QFile::WriteOnly)
            prot |= PROT_WRITE;

        mappedBuffer = (char *) mmap(NULL, deviceMaxSize, prot, MAP_SHARED, file.handle(), 0);
    }

    if (!mappedBuffer)
        qWarning(BlockDeviceLog) << "Unable to map" << file.fileName();

    return mappedBuffer;
}

qint64 BlockDevice::read(char *data, qint64 length)
{
    return file.read(data, length);
}

qint64 BlockDevice::write(const char *data, qint64 length)
{
    return file.write(data, length);
}


