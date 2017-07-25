#include <QtEndian>

#include "fring.h"
#include "fring-protocol.h"
#include "gpio.h"

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

bool Fring::initialize()
{
    if (!client.open(Fring::I2CBus, Fring::I2CAddr))
        return false;

    struct FringCommandRead rdCmd = {};
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_ID;
    if (!transfer(&wrCmd, &rdCmd))
        return false;

    if (rdCmd.id.id[0] != 'F' ||
        rdCmd.id.id[1] != 'r' ||
        rdCmd.id.id[2] != 'i' ||
        rdCmd.id.id[3] != 'n' ||
        rdCmd.id.id[4] != 'g') {
        qWarning(FringLog) << "Invalid ID code!";
        return false;
    }

    wrCmd.reg = FRING_REG_READ_BOOT_INFO;
    if (!transfer(&wrCmd, &rdCmd))
        return false;

    firmwareVersion = qFromLittleEndian(rdCmd.version.version);

    wrCmd.reg = FRING_REG_READ_BOARD_REVISION;
    if (!transfer(&wrCmd, &rdCmd))
        return false;

    boardRevisionA = rdCmd.boardRevision.boardRevisionA;
    boardRevisionB = rdCmd.boardRevision.boardRevisionB;

    qInfo(FringLog) << "Successfully initialized fring, firmware version" << firmwareVersion
                    << "board revisions" << boardRevisionA << boardRevisionB;

    // test hack
    if (firmwareVersion == 99)
        updateFirmware("/app/firmware/test.bin");

    return true;
}

bool Fring::transfer(const struct FringCommandWrite *wrCmd, const struct FringCommandRead *rdCmd)
{
    if (!client.transfer((uint8_t *) wrCmd, sizeof(*wrCmd), (uint8_t *) rdCmd, sizeof(*rdCmd))) {
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

    return transfer(&wrCmd);
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

    return transfer(&wrCmd);
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

    return transfer(&wrCmd);
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

    return transfer(&wrCmd);
}

bool Fring::readDeviceStatus()
{
    struct FringCommandRead rdCmd = {};
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_READ_DEVICE_STATUS;
    if (!transfer(&wrCmd, &rdCmd))
        return false;

    if (homeButtonState != rdCmd.deviceStatus.homeButton) {
        homeButtonState = !!rdCmd.deviceStatus.homeButton;
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
    if (!transfer(&wrCmd, &rdCmd))
        return;

    uint32_t status = qFromLittleEndian(rdCmd.interruptStatus.status);

    if (status & FRING_INTERRUPT_DEVICE_STATUS)
        readDeviceStatus();

    if (status & FRING_INTERRUPT_LOG_MESSAGE)
        readLogMessage();
}

uint32_t Fring::calculateCRC(uint32_t crc, const char *buf, size_t len)
{
    static uint32_t table[256];
    static int have_table = 0;
    uint32_t rem;
    uint8_t octet;
    int i, j;
    const char *p, *q;

    if (have_table == 0) {
        /* Calculate CRC table. */
        for (i = 0; i < 256; i++) {
            rem = i;  /* remainder from polynomial division */
            for (j = 0; j < 8; j++) {
                if (rem & 1) {
                    rem >>= 1;
                    rem ^= 0xedb88320;
                } else
                    rem >>= 1;
            }
            table[i] = rem;
        }
        have_table = 1;
    }

    crc = ~crc;
    q = buf + len;
    for (p = buf; p < q; p++) {
        octet = *p;  /* Cast to unsigned octet. */
        crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
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

    wrCmd = (struct FringCommandWrite *) alloca(sizeof(*wrCmd) + maxChunkSize);

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

        if (!transfer(wrCmd, &rdCmd))
            break;

        if (!rdCmd.updateStatus.status == FRING_UPDATE_STATUS_OK) {
            qWarning(FringLog) << "Firmware returned bad code in response to update command:" << rdCmd.updateStatus.status;
            break;
        }
    } while(r > 0);

    f.close();

    return true;
}

