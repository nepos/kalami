#pragma once

#include <QObject>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(MachineLog)

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

    enum BootConfig {
        BOOT_A,
        BOOT_B
    };

    enum Model getModel()                   const { return model;          }
    unsigned long getOsVersion()            const { return osVersion;      }
    const QString &getModelName()           const { return modelName;      }
    const QString &getArchitecture()        const { return architecture;   }
    const QString &getMachineId()           const { return machineId;      }
    const QString &getDeviceRevision()      const { return deviceRevision; }
    const QString &getDeviceSerial()        const { return deviceSerial;   }
    const QString &getCurrentBootDevice()   const { return currentBootDevice; }
    const QString &getCurrentRootfsDevice() const { return currentRootfsDevice; }
    const QString &getAltBootDevice()       const { return altBootDevice;   }
    const QString &getAltRootfsDevice()     const { return altRootfsDevice; }

    void restart();
    void powerOff();

    bool setAltBootConfig();
    bool verifyBootConfig();

private:
    enum Model model;
    enum BootConfig currentBootConfig;
    unsigned int osVersion;
    QString modelName;
    QString architecture;
    QString kernelVersion;
    QString machineId;
    QString deviceRevision;
    QString deviceSerial;
    QString currentBootDevice;
    QString currentRootfsDevice;
    QString altBootDevice;
    QString altRootfsDevice;
    QString bootDevPrefix;
};
