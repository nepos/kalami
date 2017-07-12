#include <QtEndian>

#include "fring.h"
#include "gpio.h"

Q_LOGGING_CATEGORY(FringLog, "Fring")

const int Fring::GPIONr = 8;
const int Fring::I2CBus = 0;
const int Fring::I2CAddr = 0x42;

const int FRING_REG_ID                   = 0x01;
const int FRING_REG_VERSION              = 0x02;
const int FRING_REG_BOARD_REVISION       = 0x03;
const int FRING_REG_INTERRUPT_STATUS     = 0x04;
const int FRING_REG_SET_LED              = 0x05;
const int FRING_REG_READ_DEVICE_STATUS   = 0x06;
const int FRING_REG_PUSH_FIRMWARE_UPDATE = 0x07;
const int FRING_REG_FLASH_REBOOT         = 0x08;

const int FRING_INTERRUPT_DEVICE_STATUS  = 0x01;

const int FRING_LED_MODE_OFF = 0;
const int FRING_LED_MODE_ON  = 1;
const int FRING_LED_MODE_FLASHING = 2;
const int FRING_LED_MODE_PULSATING = 3;

#define _packed_ __attribute__((__packed__))

struct FringCommandRead {
    union {
        struct {
            uint8_t id[5];
        } id;

        struct {
            uint32_t version;
        } version;

        struct {
            uint8_t boardRevisionA;
            uint8_t boardRevisionB;
        } boardRevision;

        struct {
            uint32_t status;
        } interruptStatus;

        struct {
            uint8_t batteryLevel;
            uint8_t batteryChargeCurrent;
            uint8_t batteryDischargeCurrent;
            uint8_t homeButton:1;
        } deviceStatus;

        uint8_t unused[0];
    };
}  _packed_;

struct FringCommandWrite {
    uint8_t reg;
    union {
        struct {
            uint8_t id;
            uint8_t mode;

            union {
                struct {
                } off;

                struct {
                    uint8_t r;
                    uint8_t g;
                    uint8_t b;
                } on;

                struct {
                    uint8_t r;
                    uint8_t g;
                    uint8_t b;
                    uint8_t on;
                    uint8_t off;
                } flashing;

                struct {
                    uint8_t r;
                    uint8_t g;
                    uint8_t b;
                    uint8_t period;
                } pulsating;
            };
        } led;

        uint8_t unused[0];
    };
} _packed_;

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

    wrCmd.reg = FRING_REG_VERSION;
    if (!transfer(&wrCmd, &rdCmd))
        return false;

    firmwareVersion = qFromLittleEndian(rdCmd.version.version);

    wrCmd.reg = FRING_REG_BOARD_REVISION;
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

void Fring::onInterrupt(GPIO::Value v)
{
    Q_UNUSED(v);

    struct FringCommandRead rdCmd = {};
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_INTERRUPT_STATUS;
    if (!transfer(&wrCmd, &rdCmd))
        return;

    uint32_t status = qFromLittleEndian(rdCmd.interruptStatus.status);

    if (status & FRING_INTERRUPT_DEVICE_STATUS)
        readDeviceStatus();
}

