#ifndef IMAGEREADER_H
#define IMAGEREADER_H

#include <QObject>
#include <QFile>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(ImageReaderLog)

class ImageReader : public QObject
{
    Q_OBJECT
public:

    enum ImageType {
        SquashFsType,
        AndroidBootType,
    };

    explicit ImageReader(enum ImageType type, const QString path, QObject *parent = 0);
    ~ImageReader();

    qint64 size() { return imageSize; };
    bool open();
    uchar *map();
    void close();

private:
    enum ImageType type;
    QFile file;
    qint64 imageSize;
    uchar *mappedBuffer;
};

#endif // IMAGEREADER_H
