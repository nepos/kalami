#ifndef BLOCKDEVICE_H
#define BLOCKDEVICE_H

#include <QObject>
#include <QFile>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(BlockDeviceLog)

class BlockDevice : public QObject
{
    Q_OBJECT
public:
    explicit BlockDevice(const QString path, QObject *parent = 0);
    ~BlockDevice();

    QString fileName() const { return file.fileName(); };
    qint64 maxSize() { return deviceMaxSize; };
    bool open(QFile::OpenMode m = QIODevice::ReadOnly);
    void close();
    char *map();
    qint64 read(char *data, qint64 length);
    qint64 write(const char *data, qint64 length);

private:
    QFile file;
    qint64 deviceMaxSize;
    char *mappedBuffer;
};

#endif // BLOCKDEVICE_H
