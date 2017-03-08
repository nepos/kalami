#ifndef MACHINE_H
#define MACHINE_H

#include <QObject>

class Machine : public QObject
{
    Q_OBJECT
public:
    explicit Machine(QObject *parent = 0);

    enum Model {
        UNSPECIFIED = -1,
        DEVELOPMENT = 0,
        DT410C_EVALBOARD,
        SAPHIRA,
    };

    enum Model getModel()            const { return model;        }
    unsigned long getOsVersion()     const { return osVersion;    }
    const QString &getModelName()    const { return modelName;    }
    const QString &getArchitecture() const { return architecture; }

    void restart();
    void powerOff();

private:
    enum Model model;
    QString modelName;
    QString architecture;
    QString kernelVersion;
    unsigned int osVersion;
};

#endif // MACHINE_H
