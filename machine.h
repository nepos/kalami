#pragma once

#include <QObject>
#include <QProcess>
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
        NEPOS1,
    };

    enum BootSource {
        BOOTSOURCE_INTERNAL,
        BOOTSOURCE_EXTERNAL
    };

    enum BootConfig {
        BOOTCONFIG_A,
        BOOTCONFIG_B
    };

    enum Model getModel()                   const { return model;           }
    const QString &getOsVersion()           const { return osVersion;       }
    const QString &getOsChannel()           const { return osChannel;       }
    unsigned long getOsVersionNumber()      const { return osVersionNumber; }
    const QString &getModelName()           const { return modelName;       }
    const QString &getArchitecture()        const { return architecture;    }
    const QString &getMachineId()           const { return machineId;       }
    const QString &getDeviceRevision()      const { return deviceRevision;  }
    const QString &getDeviceSerial()        const { return deviceSerial;    }
    const QString &getCurrentBootDevice()   const { return currentBootDevice; }
    const QString &getCurrentRootfsDevice() const { return currentRootfsDevice; }
    const QString &getAltBootDevice()       const { return altBootDevice;   }
    const QString &getAltRootfsDevice()     const { return altRootfsDevice; }

    void setDeviceSerial(const QString &serial);

    void suspend();
    void restart();
    void powerOff();

    bool eligibleForUpdate() const;
    bool setAltBootConfig() const;
    bool verifyBootConfig() const;

    void bootstrapInternalMemory();

signals:
    void bootstrapInternalMemoryFinished(bool success);

private:
    enum Model model;
    enum BootSource bootSource;
    enum BootConfig currentBootConfig;
    QString osVersion;
    QString osChannel;
    unsigned int osVersionNumber;
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
    QString bootConfigDevice;
    QString bootDevPrefix;
    QProcess bootstrapProcess;
};
