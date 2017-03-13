#include <QDebug>
#include <QFile>

#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <sys/utsname.h>

#include "machine.h"

Machine::Machine(QObject *parent) : QObject(parent)
{
    struct utsname uts;
    int ret;

    ret = uname(&uts);
    if (ret < 0) {
        qInfo() << "Unable to get uname():" << strerror(-errno);
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

    qInfo() << "Detected Hardware:" << architecture << "architecture,"
               << "model name" << modelName
               << "os version" << QString::number(osVersion);
}

void Machine::restart()
{
    reboot(LINUX_REBOOT_CMD_RESTART);
}

void Machine::powerOff()
{
    reboot(LINUX_REBOOT_CMD_POWER_OFF);
}
