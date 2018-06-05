#include <QDebug>
#include <QFile>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include <sys/reboot.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include "machine.h"
#include "gptparser.h"

Q_LOGGING_CATEGORY(MachineLog, "Machine")

Machine::Machine(QObject *parent) :
    QObject(parent),
    bootstrapProcess(this)
{
    QObject::connect(&bootstrapProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     [this](int exitCode, QProcess::ExitStatus exitStatus) {
        emit bootstrapInternalMemoryFinished(exitCode == 0 && exitStatus == QProcess::NormalExit);
    });
}

bool Machine::init()
{
    struct utsname uts;
    int ret;

    ret = uname(&uts);
    if (ret < 0) {
        qInfo() << "Unable to get uname():" << strerror(-errno);
        return false;
    }

    architecture = uts.machine;
    kernelVersion = uts.release;

    if (architecture == "aarch64") {
        QFile modelFile("/sys/firmware/devicetree/base/model");
        if (modelFile.open(QIODevice::ReadOnly)) {
            modelName = modelFile.readLine();
        } else {
            modelName = "Unknown arm64 device";
        }

        if (modelName.contains("APQ 8016 SBC"))
            model = Machine::DT410C_EVALBOARD;

        if (modelName.contains("NEPOS1"))
            model = Machine::NEPOS1;

    } else if (architecture == "x86_64") {
        QFile modelFile("/sys/devices/virtual/dmi/id/product_version");
        if (modelFile.open(QIODevice::ReadOnly)) {
            modelName = modelFile.readLine();
        } else {
            modelName = "Unknown arm64 device";
        }

        model = Machine::DEVELOPMENT;
    }

    while (modelName.endsWith('\n'))
        modelName.chop(1);

    osVersionNumber = 0;
    QFile os("/etc/os-release");
    if (!os.open(QIODevice::ReadOnly)) {
        qWarning(MachineLog) << "Unable to open" << os.fileName() << os.errorString();
        return false;
    }

    while (!os.atEnd()) {
        QString line = os.readLine();

        while (line.endsWith('\n') || line.endsWith('"'))
            line.chop(1);

        if (line.startsWith("VERSION_ID=")) {
            osVersion = line.mid(12);
            QStringList l = osVersion.split("-");

            if (l.length() == 2) {
                osChannel = l[0];
                osVersionNumber = l[1].toULong();
            } else {
                qWarning(MachineLog) << "Unable to parse VERSION_ID field in /etc/os-release" << osVersion;
            }
        }
    }

    os.close();

    QFile machineIdFile("/etc/machine-id");
    if (machineIdFile.open(QIODevice::ReadOnly)) {
        machineId = machineIdFile.readLine();
        machineIdFile.close();

        while (machineId.endsWith('\n'))
            machineId.chop(1);
    }

    deviceRevision = "a";
    deviceSerial = "xxx";

    qInfo(MachineLog) << "Detected Hardware:" << architecture << "architecture,"
                      << "model name" << modelName
                      << "revision" << deviceRevision
                      << "os channel" << osChannel
                      << "os version" << QString::number(osVersionNumber)
                      << "serial" << deviceSerial;

    // skip rootfs detection logic for development machines
    if (model != Machine::NEPOS1)
        return true;

    // We need to find out which device we were booted from. We know it's a mmc block
    // device, but as we boot by UUID, we don't know whether it's /dev/mmcblk0 or /dev/mmcblk1.

    struct stat statbuf;
    ret = stat("/", &statbuf);
    if (ret < 0) {
        qWarning(MachineLog) << "Error stat()ing / !";
        return false;
    }

    QString p = QString::asprintf("/sys/dev/block/%d:%d/uevent",
                                  major(statbuf.st_dev), minor(statbuf.st_dev));
    QFile uevent(p);

    if (!uevent.open(QIODevice::ReadOnly)) {
        qWarning(MachineLog) << "Unable to open" << uevent.fileName() << uevent.errorString();
        return false;
    } else {
        for (;;) {
            QString line = uevent.readLine();
            if (line.isEmpty())
                break;

            QStringList parts = line.split("=");
            if (parts.count() != 2)
                continue;

            if (parts[0] == "DEVNAME")
                bootDevPrefix = QString("/dev/%1").arg(parts[1].mid(0, strlen("mmcblkX")));
        }

        uevent.close();
    }

    QString currentRootfsUUID;

    QFile cmdlineFile("/proc/cmdline");
    if (!cmdlineFile.open(QIODevice::ReadOnly)) {
        qWarning(MachineLog) << "Unable to open" << cmdlineFile.fileName() << cmdlineFile.errorString();
        return false;
    } else {
        QStringList cmdline = QString(cmdlineFile.readLine().data()).split(' ');
        cmdlineFile.close();

        for (int i = 0; i < cmdline.size(); ++i) {
            QString c = cmdline.at(i);

            if (c.startsWith("root=PARTUUID="))
                currentRootfsUUID = c.mid(strlen("root=PARTUUID="));
        }
    }

    if (currentRootfsUUID == "5ee54693-b9c5-4367-a52e-3361c165c800") {
        bootSource = BootSource::BOOTSOURCE_INTERNAL;
        currentBootConfig = Machine::BOOTCONFIG_A;
    }

    if (currentRootfsUUID == "5ee54693-b9c5-4367-a52e-3361c165c801") {
        bootSource = BootSource::BOOTSOURCE_INTERNAL;
        currentBootConfig = Machine::BOOTCONFIG_B;
    }

    if (currentRootfsUUID == "5ee54693-b9c5-4367-a52e-3361c165c810") {
        bootSource = BootSource::BOOTSOURCE_EXTERNAL;
        currentBootConfig = Machine::BOOTCONFIG_A;
    }

    if (currentRootfsUUID == "5ee54693-b9c5-4367-a52e-3361c165c811") {
        bootSource = BootSource::BOOTSOURCE_EXTERNAL;
        currentBootConfig = Machine::BOOTCONFIG_B;
    }

    GPTParser parser(bootDevPrefix);

    if (currentBootConfig == Machine::BOOTCONFIG_A) {
        currentRootfsDevice = parser.deviceNameForPartitionName("rootfs-a");
        currentBootDevice = parser.deviceNameForPartitionName("boot-a");
        altRootfsDevice = parser.deviceNameForPartitionName("rootfs-b");
        altBootDevice = parser.deviceNameForPartitionName("boot-b");
    } else {
        currentRootfsDevice = parser.deviceNameForPartitionName("rootfs-b");
        currentBootDevice = parser.deviceNameForPartitionName("boot-b");
        altRootfsDevice = parser.deviceNameForPartitionName("rootfs-a");
        altBootDevice = parser.deviceNameForPartitionName("boot-a");
    }

    bootConfigDevice = parser.deviceNameForPartitionName("bootcfg");

    qInfo(MachineLog) << "Boot config" << (currentBootConfig == Machine::BOOTCONFIG_A ? "A" : "B");
    qInfo(MachineLog) << "Current rootfs on" << currentRootfsDevice << "boot image on" << currentBootDevice;
    qInfo(MachineLog) << "Alt rootfs on" << altRootfsDevice << "alt boot image on" << altBootDevice;

    return true;
}

void Machine::setDeviceSerial(const QString &serial)
{
    deviceSerial = serial;
    qInfo(MachineLog) << "Device serial set to" << serial;
}

bool Machine::eligibleForUpdate() const
{
//    Make sure to enable this check before release!
//    if (bootSource != BootSource::BOOTSOURCE_INTERNAL)
//        return false;

    return true;
}

bool Machine::setAltBootConfig() const
{
    if (model != Machine::NEPOS1)
        return false;

    QFile bootConfigFile(bootConfigDevice);
    if (!bootConfigFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
        return false;

    // When changing the boot config, use the lower-case letter. The bootloader will
    // turn it into the upper-case version and negative it, but still boot into the
    // alternative rootfs. verifyBootConfig() can detect this mismatch and make the
    // new version permanent.

    char b = currentBootConfig == Machine::BOOTCONFIG_A ? 'b' : 'a';
    bootConfigFile.write(&b, sizeof(b));
    bootConfigFile.close();

    return true;
}

bool Machine::isTentativeBoot() const
{
    QFile bootConfigFile(bootConfigDevice);

    if (!bootConfigFile.exists())
        return false;

    if (!bootConfigFile.open(QIODevice::ReadOnly))
        return false;

    char b;
    bootConfigFile.read(&b, sizeof(b));
    bootConfigFile.reset();

    if (b == 'A' && currentBootConfig != Machine::BOOTCONFIG_A)
        return true;

    if (b == 'B' && currentBootConfig != Machine::BOOTCONFIG_B)
        return true;

    return false;
}

bool Machine::verifyBootConfig() const
{
    QFile bootConfigFile(bootConfigDevice);

    if (!bootConfigFile.exists())
        return false;

    if (!bootConfigFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    char b;
    bootConfigFile.read(&b, sizeof(b));
    bootConfigFile.reset();

    // Only take action in case the currently booted rootfs differers from
    // the one configured in the boot config partition. If that's the case,
    // the machine was recently updated, and the bootloader reverted the
    // boot configuration before booting into the current image.

    if (b == 'A' && currentBootConfig != Machine::BOOTCONFIG_A) {
        b = 'B';
        qInfo(MachineLog) << "Making boot B permanent";
        bootConfigFile.write(&b, sizeof(b));
    } else if (b == 'B' && currentBootConfig != Machine::BOOTCONFIG_B) {
        b = 'A';
        qInfo(MachineLog) << "Making boot A permanent";
        bootConfigFile.write(&b, sizeof(b));
    }

    bootConfigFile.close();

    return true;
}

void Machine::bootstrapInternalMemory()
{
    bootstrapProcess.kill();
    bootstrapProcess.start("/app/bin/bootstrap.sh", QStringList());
}

void Machine::suspend()
{
    QFile state("/sys/power/state");
    state.open(QFile::WriteOnly | QFile::Unbuffered);
    qInfo(MachineLog) << "Entering suspend";
    state.write("freeze");
    qInfo(MachineLog) << "Returned from suspend";
    state.close();
}

void Machine::restart()
{
    QDBusMessage m =
            QDBusMessage::createMethodCall("org.freedesktop.systemd1",
                                           "/org/freedesktop/systemd1",
                                           "org.freedesktop.systemd1.Manager",
                                           "Reboot");
    QDBusConnection::systemBus().call(m);
}

void Machine::powerOff()
{
    QDBusMessage m =
            QDBusMessage::createMethodCall("org.freedesktop.systemd1",
                                           "/org/freedesktop/systemd1",
                                           "org.freedesktop.systemd1.Manager",
                                           "PowerOff");
    QDBusConnection::systemBus().call(m);
}
