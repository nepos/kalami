#ifndef IMAGEREADER_H
#define IMAGEREADER_H

#include <QObject>
#include <QFile>
#include <QtCore/QLoggingCategory>

#include "blockdevice.h"

Q_DECLARE_LOGGING_CATEGORY(ImageReaderLog)

class ImageReader : public BlockDevice
{
    Q_OBJECT
public:

    enum ImageType {
        SquashFsType,
        AndroidBootType,
    };

    explicit ImageReader(enum ImageType type, const QString path, QObject *parent = 0);

    qint64 size() { return imageSize; };
    bool open();

private:
    enum ImageType type;
    qint64 imageSize;
};

#endif // IMAGEREADER_H
