#include <QtEndian>
#include <QDir>

#include "fring.h"
#include "fring-protocol.h"
#include "gpio.h"
#include "crc32table.h"

Q_LOGGING_CATEGORY(FringLog, "Fring")

const int Fring::GPIONr = 8;
const int Fring::I2CBus = 0;
const int Fring::I2CAddr = 0x42;

Fring::Fring(QObject *parent) :
    QObject(parent),
    client(this),
    interruptGpio(Fring::GPIONr, this)
{
    interruptGpio.setEdge(GPIO::EdgeFalling);
    interruptGpio.setDirection(GPIO::DirectionIn);
    QObject::connect(&interruptGpio, &GPIO::onDataReady, this, &Fring::onInterrupt);

    homeButtonState = -1;
}

#define CMD_LEN(type, sub) \
({ \
    type __x; \
    (offsetof(type, sub) + sizeof(__x.sub)); \
})

bool Fring::initialize()
{
    if (!client.open(Fring::I2CBus, Fring::I2CAddr))
        return false;

    struct FringCommandRead rdCmd = {};
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_ID;
    wrCmd.protocolVersion.version = 1;
    if (!transfer(&wrCmd, CMD_LEN(struct FringCommandWrite, protocolVersion),
                  &rdCmd, CMD_LEN(struct FringCommandRead, id)))
        return false;

    if (rdCmd.id.id[0] != 'F' ||
        rdCmd.id.id[1] != 'r' ||
        rdCmd.id.id[2] != 'i' ||
        rdCmd.id.id[3] != 'n' ||
        rdCmd.id.id[4] != 'g') {
        qWarning(FringLog) << "Invalid ID code " << QByteArray((char *) rdCmd.id.id, 5);
        return false;
    }

    QStringList bootFlagsStrings;

    wrCmd.reg = FRING_REG_READ_BOOT_INFO;
    if (!transfer(&wrCmd, 1, &rdCmd, sizeof(rdCmd.bootInfo)))
        return false;

    uint32_t bootFlags = qFromLittleEndian(rdCmd.bootInfo.flags);

    if (bootFlags & FRING_BOOT_STATUS_FIRMWARE_B)
        bootFlagsStrings << "booted from B";
    else
        bootFlagsStrings << "booted from A";

    if (bootFlags & FRING_BOOT_STATUS_BETA)
        bootFlagsStrings << "beta version";

    firmwareVersion = qFromLittleEndian(rdCmd.bootInfo.version);

    wrCmd.reg = FRING_REG_READ_BOARD_REVISION;
    if (!transfer(&wrCmd, 1, &rdCmd, sizeof(rdCmd.boardRevision)))
        return false;

    boardRevisionA = rdCmd.boardRevision.boardRevisionA;
    boardRevisionB = rdCmd.boardRevision.boardRevisionB;

    qInfo(FringLog) << "Successfully initialized fring, firmware version" << firmwareVersion
                    << QString("(" + bootFlagsStrings.join(", ") + ")")
                    << "board revisions" << boardRevisionA << boardRevisionB;

    // Check for firmware updates
    QDir firmwareDir = QDir("/app/firmware/fring/");
    QStringList firmwareFiles = firmwareDir.entryList(QDir::Files);

    if (!firmwareFiles.isEmpty()) {
        QString firmwareFile = firmwareFiles.first();
        QStringList parts = firmwareFile.split(".");

        if (!parts.isEmpty()) {
            int availableVersion = parts.first().toInt();

            if (availableVersion > firmwareVersion) {
                qInfo(FringLog) << "Newer firmware available (" + firmwareFile + "), updating.";
                updateFirmware(firmwareDir.absolutePath() + "/" + firmwareFile);
            }
        }
    }

    return true;
}

bool Fring::transfer(const struct FringCommandWrite *wrCmd, size_t wrSize,
                     const struct FringCommandRead *rdCmd, size_t rdSize)
{
    if (!client.transfer((uint8_t *) wrCmd, wrSize, (uint8_t *) rdCmd, rdSize)) {
        qWarning(FringLog) << "Unable to transfer command!";
        return false;
    }

    return true;
}

bool Fring::setLedOff(int id)
{
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_SET_LED;
    wrCmd.led.id = id;
    wrCmd.led.mode = FRING_LED_MODE_OFF;

    return transfer(&wrCmd, offsetof(struct FringCommandWrite, led) + sizeof(wrCmd.led));
}

bool Fring::setLedOn(int id, float r, float g, float b)
{
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_SET_LED;
    wrCmd.led.id = id;
    wrCmd.led.mode = FRING_LED_MODE_ON;
    wrCmd.led.on.r = 255.0f / r;
    wrCmd.led.on.g = 255.0f / g;
    wrCmd.led.on.b = 255.0f / b;

    return transfer(&wrCmd, offsetof(struct FringCommandWrite, led) + sizeof(wrCmd.led));
}

bool Fring::setLedFlashing(int id, float r, float g, float b, float onPhase, float offPhase)
{
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_SET_LED;
    wrCmd.led.id = id;
    wrCmd.led.mode = FRING_LED_MODE_FLASHING;
    wrCmd.led.flashing.r = 255.0f / r;
    wrCmd.led.flashing.g = 255.0f / g;
    wrCmd.led.flashing.b = 255.0f / b;
    wrCmd.led.flashing.on = onPhase / 0.01f;
    wrCmd.led.flashing.off = offPhase / 0.01f;

    return transfer(&wrCmd, offsetof(struct FringCommandWrite, led) + sizeof(wrCmd.led));
}

bool Fring::setLedPulsating(int id, float r, float g, float b, float period)
{
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_SET_LED;
    wrCmd.led.id = id;
    wrCmd.led.mode = FRING_LED_MODE_FLASHING;
    wrCmd.led.pulsating.r = 255.0f / r;
    wrCmd.led.pulsating.g = 255.0f / g;
    wrCmd.led.pulsating.b = 255.0f / b;
    wrCmd.led.pulsating.period = period * 100.0f;

    return transfer(&wrCmd, offsetof(struct FringCommandWrite, led) + sizeof(wrCmd.led));
}

bool Fring::readDeviceStatus()
{
    struct FringCommandRead rdCmd = {};
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_READ_DEVICE_STATUS;
    if (!transfer(&wrCmd, 1, &rdCmd, sizeof(rdCmd.deviceStatus)))
        return false;

    uint32_t status = qFromLittleEndian(rdCmd.deviceStatus.status);

    bool home = !!(status & FRING_DEVICE_STATUS_HOME_BUTTON);

    if (homeButtonState != home) {
        homeButtonState = home;
        emit homeButtonChanged(homeButtonState);
    }

    if (batteryLevel != rdCmd.deviceStatus.batteryLevel ||
        batteryChargeCurrent != rdCmd.deviceStatus.batteryChargeCurrent ||
        batteryDischargeCurrent != rdCmd.deviceStatus.batteryDischargeCurrent) {
        batteryLevel = rdCmd.deviceStatus.batteryLevel;
        batteryChargeCurrent = rdCmd.deviceStatus.batteryChargeCurrent;
        batteryDischargeCurrent = rdCmd.deviceStatus.batteryDischargeCurrent;

        emit batteryStateChanged(255.0 / batteryLevel, (float) batteryChargeCurrent, (float) batteryDischargeCurrent);
    }

    return true;
}

bool Fring::readLogMessage()
{
    struct FringCommandWrite wrCmd = {};
    char buf[256];

    wrCmd.reg = FRING_REG_READ_LOG_MESSAGE;

    if (!client.transfer((uint8_t *) &wrCmd, sizeof(wrCmd), (uint8_t *) buf, sizeof(buf))) {
        qWarning(FringLog) << "Unable to transfer command!";
        return false;
    }

    emit logMessageReceived(QString(buf));

    return true;
}

void Fring::onInterrupt(GPIO::Value v)
{
    Q_UNUSED(v);

    struct FringCommandRead rdCmd = {};
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_READ_INTERRUPT_STATUS;
    if (!transfer(&wrCmd, 1, &rdCmd, sizeof(rdCmd.interruptStatus)))
        return;

    uint32_t status = qFromLittleEndian(rdCmd.interruptStatus.status);

    if (status & FRING_INTERRUPT_DEVICE_STATUS)
        readDeviceStatus();

    if (status & FRING_INTERRUPT_LOG_MESSAGE)
        readLogMessage();
}

uint32_t Fring::calculateCRC(uint32_t crc, const char *buf, size_t len)
{
    const char *p;
    uint8_t octet;

    crc ^= ~0U;

    for (p = buf; p < buf + len; p++) {
        octet = *p;  /* Cast to unsigned octet. */
        crc = (crc >> 8) ^ crc32table[(crc & 0xff) ^ octet];
    }

    return ~crc;
}

bool Fring::updateFirmware(const QString filename)
{
    uint32_t fullCRC = 0;
    uint32_t offset = 0;
    qint64 r;
    QFile f(filename);
    static const size_t maxChunkSize = 1024;
    struct FringCommandRead rdCmd;
    struct FringCommandWrite *wrCmd;

    size_t wrSize =
            offsetof(struct FringCommandWrite, firmwareUpdate)
            + sizeof(wrCmd->firmwareUpdate)
            + maxChunkSize;
    wrCmd = (struct FringCommandWrite *) alloca(wrSize);

    if (!f.open(QFile::ReadOnly)) {
        qWarning(FringLog()) << "Unable to open file" << filename;
        return false;
    }

    wrCmd->reg = FRING_REG_PUSH_FIRMWARE_UPDATE;

    qInfo(FringLog) << "Transmitting firmware file" << f.fileName() << "size" << f.size();

    do {
        r = f.read(wrCmd->firmwareUpdate.payload, maxChunkSize);
        if (r < 0) {
            qWarning(FringLog) << "Unable to read firmware file!";
            break;
        }

        wrCmd->firmwareUpdate.length = qToLittleEndian(r);

        if (r == 0) {
            wrCmd->firmwareUpdate.offset = 0;
            wrCmd->firmwareUpdate.crc = qToLittleEndian(fullCRC);
        } else {
            wrCmd->firmwareUpdate.offset = qToLittleEndian(offset);
            wrCmd->firmwareUpdate.crc = qToLittleEndian(calculateCRC(0, wrCmd->firmwareUpdate.payload, r));
            fullCRC = calculateCRC(fullCRC, wrCmd->firmwareUpdate.payload, r);
            offset += r;
        }

        if (!transfer(wrCmd, wrSize, &rdCmd, sizeof(rdCmd.updateStatus)))
            break;

        if (!rdCmd.updateStatus.status == FRING_UPDATE_STATUS_OK) {
            qWarning(FringLog) << "Firmware returned bad code in response to update command:" << rdCmd.updateStatus.status;
            break;
        }
    } while(r > 0);

    f.close();

    return true;
}

