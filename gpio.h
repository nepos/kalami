#ifndef GPIO_H
#define GPIO_H

#include <QObject>
#include <QtCore/QLoggingCategory>
#include <QFile>

Q_DECLARE_LOGGING_CATEGORY(GPIOLog)

class GPIO : public QObject
{
    Q_OBJECT
public:
    explicit GPIO(int number, QObject *parent = 0);
    ~GPIO();

    enum Direction {
        DirectionIn  = 0x01,
        DirectionOut = 0x02
    };

    enum Edge {
        EdgeRising   = 0x01,
        EdgeFalling  = 0x02,
        EdgeBoth     = 0x03
    };

    enum Value {
        ValueHi      = '1',
        ValueLo      = '0'
    };

signals:
    void onDataReady(int fd);

public slots:
    void set(GPIO::Value v);
    void setDirection(Direction io);
    void setEdge(Edge e);

private:
    Q_DISABLE_COPY(GPIO)
    void openValueFile(QFile::OpenModeFlag f);
    void closeValueFile();

    const static QString basePath;
    QString gpioPath;

    int number;
    QString pathExport;
    QString pathUnexport;
    Direction direction;

    QFile valueFile;
};

#endif // GPIO_H
