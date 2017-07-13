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

    qInfo() << "Successfully initialized fring, firmware version" << firmwareVersion
            << "board revisions" << boardRevisionA << boardRevisionB;

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

