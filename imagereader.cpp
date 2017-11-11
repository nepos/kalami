#include <QtEndian>
#include <QDebug>

#include <linux/magic.h>
#include "imagereader.h"

Q_LOGGING_CATEGORY(ImageReaderLog, "ImageReader")

struct squashfsHeader {
    uint32_t s_magic;
    uint32_t inodes;
    uint32_t mkfs_time;
    uint32_t block_size;
    uint32_t fragments;
    uint16_t compression;
    uint16_t block_log;
    uint16_t flags;
    uint16_t no_ids;
    uint16_t s_major;
    uint16_t s_minor;
    uint64_t root_inode;
    uint64_t bytes_used;

    /* ignore the rest */
} __attribute__((__packed__));

struct androidBootHeader {
    uint32_t magic;
    uint32_t magic2;

    uint32_t kernel_size;
    uint32_t kernel_addr;

    uint32_t initrd_size;
    uint32_t initrd_addr;

    uint32_t second_size;
    uint32_t second_addr;

    uint32_t tags_addr;
    uint32_t page_size;

    uint32_t dtb_size;

        /* ignore the rest */
} __attribute__((__packed__));

#define _ANDROID_BOOTIMG_MAGIC_1 UINT32_C(0x52444e41)
#define _ANDROID_BOOTIMG_MAGIC_2 UINT32_C(0x2144494f)

ImageReader::ImageReader(enum ImageType type, const QString path, QObject *parent) :
    QObject(parent), type(type), file(path), mappedBuffer(0)
{
}

ImageReader::~ImageReader()
{
    close();
}

static inline size_t ALIGN_TO(size_t l, size_t ali) {
    return ((l + ali - 1) & ~(ali - 1));
}

bool ImageReader::open()
{
    if (!file.open(QIODevice::ReadOnly))
        return false;

    switch (type) {
    case ImageReader::SquashFsType: {
        struct squashfsHeader hdr;

        qint64 r = file.read((char *) &hdr, sizeof(hdr));
        if (r != sizeof(hdr)) {
            qWarning(ImageReaderLog) << "Unable to read header of" << file.fileName();
            return false;
        }

        if (qFromLittleEndian(hdr.s_magic) != SQUASHFS_MAGIC) {
            qWarning(ImageReaderLog) << "Wrong superblock magic in" << file.fileName();
            return false;
        }

        imageSize = ALIGN_TO(qFromLittleEndian(hdr.bytes_used), 4096ULL);
        break;
    }

    case ImageReader::AndroidBootType: {
        struct androidBootHeader hdr;

        qint64 r = file.read((char *) &hdr, sizeof(hdr));
        if (r != sizeof(hdr)) {
            qWarning(ImageReaderLog) << "Unable to read header of" << file.fileName();
            return false;
        }

        if (qFromLittleEndian(hdr.magic) != _ANDROID_BOOTIMG_MAGIC_1 ||
            qFromLittleEndian(hdr.magic2) != _ANDROID_BOOTIMG_MAGIC_2) {
            qWarning(ImageReaderLog) << "Wrong superblock magic!";
            return false;
        }

        uint32_t pagesize = qFromLittleEndian(hdr.page_size);

        imageSize =
                (uint64_t) ALIGN_TO(608, pagesize) /* header size */ +
                (uint64_t) ALIGN_TO(le32toh(hdr.kernel_size), pagesize) +
                (uint64_t) ALIGN_TO(le32toh(hdr.initrd_size), pagesize) +
                (uint64_t) ALIGN_TO(le32toh(hdr.second_size), pagesize) +
                (uint64_t) ALIGN_TO(le32toh(hdr.dtb_size), pagesize);
        break;
    }

    default:
        qWarning(ImageReaderLog) << "Unsupported image type in" << file.fileName();
        return false;
    }

    return true;
}

void ImageReader::close()
{
    if (!file.isOpen())
        return;

    if (mappedBuffer) {
        file.unmap(mappedBuffer);
        mappedBuffer = nullptr;
    }

    file.close();
}

uchar *ImageReader::map()
{
    if (!file.isOpen())
        return NULL;

    if (!mappedBuffer)
        mappedBuffer = file.map(0, imageSize, QFileDevice::NoOptions);

    return mappedBuffer;
}
