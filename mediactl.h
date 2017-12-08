#ifndef MEDIACTL_H
#define MEDIACTL_H

#include <QObject>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(MediaCtlLog)

class MediaCtl : public QObject
{
    Q_OBJECT
public:
    explicit MediaCtl(int index = 0, QObject *parent = 0);

    enum Config {
        UYVY8_2X8_1920x1080,
        UYVY8_2X8_2592x1944,
    };

    bool initialize();
    bool setConfig(int index, enum Config config);

private:
    QString mediaDevice;
    QStringList sensorNames;
    bool invokeBinary(const QStringList extraArgs);
    bool reset();
    bool makeLinks();
};

#endif // MEDIACTL_H
