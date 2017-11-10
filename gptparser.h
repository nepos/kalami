#pragma once

#include <QObject>
#include <QFile>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GPTParserLog)

class GPTParser : public QObject
{
    Q_OBJECT
public:
    explicit GPTParser(const QString &deviceName, QObject *parent = 0);
    ~GPTParser();

    int partitionIndexForName(const QString &name);
    const QString nameOfPartition(unsigned int index);
    const QString deviceNameForPartitionName(const QString &partitionName);

private:
    bool isMMC;
    QString baseDevice;
    QFile file;
    uint32_t numEntries;
};

