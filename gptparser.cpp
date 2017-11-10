#include <QDebug>
#include "gptparser.h"

Q_LOGGING_CATEGORY(GPTParserLog, "GPTParser")

struct GPTHeader {
    uint64_t signature;
    uint32_t revision;
    uint8_t ignore[68];
    uint32_t numEntries;
    // ignore the rest
};

struct GPTEntry {
    uint8_t typeUUID[16];
    uint8_t UUID[16];
    uint8_t ignore[24];
    char16_t name[36];
};

GPTParser::GPTParser(const QString &deviceName, QObject *parent) :
    QObject(parent), file(deviceName)
{
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning(GPTParserLog) << "Error opening" << deviceName;
        return;
    }

    struct GPTHeader header;
    file.seek(512);
    file.read((char *) &header, sizeof(header));

    if (header.signature != 0x5452415020494645ULL) {
        qWarning(GPTParserLog) << "Invalid GPT header signature, closing." << deviceName;
        file.close();
        return;
    }

    isMMC = deviceName.contains("mmcblk");

    baseDevice = deviceName;
    numEntries = header.numEntries;
}

GPTParser::GPTParser::~GPTParser()
{
    if (file.isOpen())
        file.close();
}

int GPTParser::partitionIndexForName(const QString &name)
{
    if (!file.isOpen())
        return -1;

    for (uint32_t i = 0; i < numEntries; ++i) {
        struct GPTEntry entry;

        file.seek(512 + 512 + i * 128);
        file.read((char *) &entry, sizeof(entry));

        if (name == QString::fromUtf16(entry.name))
            return i;
    }

    return -1;
}

const QString GPTParser::deviceNameForPartitionName(const QString &partitionName)
{
    QString device = baseDevice;

    if (isMMC)
        device += "p";

    device += QString::number(partitionIndexForName(partitionName));

    return device;
}

const QString GPTParser::nameOfPartition(unsigned int index)
{
    if (!file.isOpen() || index >= numEntries)
        return "";

    struct GPTEntry entry;

    file.seek(512 + 512 + index * 128);
    file.read((char *) &entry, sizeof(entry));

    return QString::fromUtf16(entry.name);
}

