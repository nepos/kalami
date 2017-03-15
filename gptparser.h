#ifndef GPTPARSER_H
#define GPTPARSER_H

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

    int findPartitionByName(const QString &name);

signals:

public slots:

private:
    QFile file;
    uint32_t numEntries;
};

#endif // GPTPARSER_H
