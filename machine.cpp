#include <QDebug>
#include <QFile>

#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <sys/utsname.h>

#include "machine.h"
#include "gptparser.h"

Q_LOGGING_CATEGORY(MachineLog, "Machine")

Machine::Machine(QObject *parent) : QObject(parent)
{
    struct utsname uts;
    int ret;

    ret = uname(&uts);
    if (ret < 0)
        qInfo() << "Unable to get uname():" << strerror(-errno);

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

    osVersion = 0;
    QFile os("/etc/os-release");
    if (os.open(QIODevice::ReadOnly)) {
        while (!os.atEnd()) {
            QString line = os.readLine();

            while (line.endsWith('\n') || line.endsWith('"'))
                line.chop(1);

            if (line.startsWith("VERSION_ID="))
                osVersion = line.mid(12).toUInt();
        }

        os.close();
    }

    QFile machineIdFile("/etc/machine-id");
    if (machineIdFile.open(QIODevice::ReadOnly)) {
        machineId = machineIdFile.readLine();
        machineIdFile.close();

        while (machineId.endsWith('\n'))
            machineId.chop(1);
    }

    deviceRevision = "a";
    deviceSerial = "xxx";

    qInfo() << "Detected Hardware:" << architecture << "architecture,"
               << "model name" << modelName
               << "revision" << deviceRevision
               << "os version" << QString::number(osVersion)
               << "serial" << deviceSerial;

    QFile cmdlineFile("/proc/cmdline");
    if (cmdlineFile.open(QIODevice::ReadOnly)) {
        QStringList cmdline = QString(cmdlineFile.readLine().data()).split(' ');
        cmdlineFile.close();

        for (int i = 0; i < cmdline.size(); ++i) {
            QString c = cmdline.at(i);

            if (c.startsWith("root="))
                currentRootfsDevice = c.mid(5);
        }
    }

    bootDevPrefix = currentRootfsDevice.mid(0, strlen("/dev/mmcblk0"));

    //GPTParser parser(bootDevPrefix);

    if (currentRootfsDevice.endsWith("p12")) {
        altRootfsDevice = bootDevPrefix + "p13";
        currentBootDevice = bootDevPrefix + "p10";
        altBootDevice = bootDevPrefix + "p11";
        currentBootConfig = Machine::BOOT_A;
    } else {
        altRootfsDevice = bootDevPrefix + "p12";
        currentBootDevice = bootDevPrefix + "p11";
        altBootDevice = bootDevPrefix + "p10";
        currentBootConfig = Machine::BOOT_B;
    }
}

void Machine::setDeviceSerial(const QString &serial)
{
    deviceSerial = serial;
    qInfo(MachineLog) << "Device serial set to" << serial;
}

bool Machine::setAltBootConfig()
{
    QFile bootConfigFile(bootDevPrefix + "p7");
    if (!bootConfigFile.open(QIODevice::WriteOnly))
        return false;

    char b = currentBootConfig == Machine::BOOT_A ? 'b' : 'a';

    bootConfigFile.write(&b, sizeof(b));
    bootConfigFile.close();

    return true;
}

bool Machine::verifyBootConfig()
{
    QFile bootConfigFile(bootDevPrefix + "p7");
    if (!bootConfigFile.open(QIODevice::ReadWrite))
        return false;

    char b;
    bootConfigFile.read(&b, sizeof(b));
    bootConfigFile.reset();

    if (b == 'a') {
        b = 'A';
        bootConfigFile.write(&b, sizeof(b));
    } else if (b == 'b') {
        b = 'B';
        bootConfigFile.write(&b, sizeof(b));
    }

    bootConfigFile.close();

    return true;
}

void Machine::restart()
{
    reboot(LINUX_REBOOT_CMD_RESTART);
}

void Machine::powerOff()
{
    reboot(LINUX_REBOOT_CMD_POWER_OFF);
}
