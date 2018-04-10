#ifndef FRINGPROTOCOL_H
#define FRINGPROTOCOL_H

namespace FringProtocol {

enum {
    FRING_REG_ID                          = 0x01,
    FRING_REG_READ_BOOT_INFO              = 0x02,
    FRING_REG_READ_BOARD_REVISION         = 0x03,
    FRING_REG_READ_INTERRUPT_STATUS       = 0x04,
    FRING_REG_SET_LED                     = 0x05,
    FRING_REG_READ_DEVICE_STATUS          = 0x06,
    FRING_REG_READ_BATTERY_STATUS         = 0x07,
    FRING_REG_PUSH_FIRMWARE_UPDATE        = 0x08,
    FRING_REG_READ_FIRMWARE_UPDATE_RESULT = 0x09,
    FRING_REG_READ_LOG_MESSAGE            = 0x0a,
    FRING_REG_SET_SERIAL                  = 0x0b,
    FRING_REG_SET_WAKEUP_TIME             = 0x0c,
    FRING_REG_READ_WAKEUP_REASON          = 0x0d,
};

enum {
    FRING_HWERR_LED_NOT_RESPONDING        = 0x01,
    FRING_HWERR_CHARGER_NOT_RESPONDING    = 0x02,
    FRING_HWERR_CHARGER_INIT_ERROR        = 0x04,
    FRING_HWERR_CHARGER_OVERTEMPERATURE   = 0x08,
    FRING_HWERR_USB_NOT_RESPONDING        = 0x10,
    FRING_HWERR_USB_INIT_ERROR            = 0x20,
    FRING_HWERR_BATTERY_NOT_RESPONDING    = 0x40,
    FRING_HWERR_BATTERY_INIT_ERROR        = 0x80,
    FRING_HWERR_BATTERY_OVERTEMPERATURE   = 0x100,
    FRING_HWERR_ADC                       = 0x200,
};

enum {
    FRING_BOOT_STATUS_FIRMWARE_B          = 0x01,
    FRING_BOOT_STATUS_BETA                = 0x02,
};

enum {
    FRING_DEVICE_STATUS_HOME_BUTTON     = 0x01
};

enum {
    FRING_INTERRUPT_DEVICE_STATUS   = 0x01,
    FRING_INTERRUPT_BATTERY_STATUS  = 0x02,
    FRING_INTERRUPT_LOG_MESSAGE     = 0x04,
    FRING_INTERRUPT_FIRMWARE_UPDATE = 0x08,
    FRING_INTERRUPT_WAKEUP      = 0x10
};

enum {
    FRING_LED_MODE_OFF              = 0,
    FRING_LED_MODE_ON               = 1,
    FRING_LED_MODE_FLASHING         = 2,
    FRING_LED_MODE_PULSATING        = 3
};

enum {
    FRING_UPDATE_RESULT_OK          = 0,
    FRING_UPDATE_RESULT_CRC_ERR     = 1,
    FRING_UPDATE_RESULT_INVAL       = 2,
    FRING_UPDATE_RESULT_INTERNAL_ERR= 3
};

#define _packed_ __attribute__((__packed__))

struct Id {
    uint8_t id[5];
} _packed_;

struct BootInfo {
    uint32_t version;
    uint32_t uptime;
    uint32_t flags;
    uint8_t serial[12];
} _packed_;

struct BoardRevision {
    uint8_t hardwareType;
    uint8_t boardRevisionA;
    uint8_t boardRevisionB;
    uint8_t dummy;
} _packed_;

struct InterruptStatus {
    uint32_t status;
} _packed_;

struct DeviceStatus {
    uint32_t status;
    uint32_t hardwareErrors;
    uint8_t ambientLightValue;
    uint8_t temp0;
    uint8_t temp1;
    uint8_t temp2;
} _packed_;

struct BatteryStatus {
    int8_t chargeCurrent;
    uint8_t level;
    uint8_t temp;
    uint8_t dummy;
    uint16_t status;
    uint16_t remainingCapacity;
    uint16_t averageTimeToEmpty;
    uint16_t averageTimeToFull;
    uint16_t cycleCount;
} _packed_;

struct UpdateStatus {
    uint32_t status;
} _packed_;

struct WakeupReason {
    uint32_t reason;
} _packed_;

struct CommandRead {
    union {
        Id id;
        BootInfo bootInfo;
        BoardRevision boardRevision;
        InterruptStatus interruptStatus;
        DeviceStatus deviceStatus;
        BatteryStatus batteryStatus;
        UpdateStatus updateStatus;
        WakeupReason wakeupReason;

        uint8_t unused[0];
    };
} _packed_;

struct Protocol {
    uint8_t version;
} _packed_;

struct Off {
} _packed_;

struct On {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} _packed_;

struct Flashing {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t on;
    uint8_t off;
} _packed_;

struct Pulsating {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t period;
} _packed_;

struct Led {
    uint8_t id;
    uint8_t mode;

    union {
        Off off;
        On on;
        Flashing flashing;
        Pulsating pulsating;
    };
} _packed_;

struct FirmwareUpdate {
    uint32_t length;
    uint32_t offset;
    uint32_t crc;
    char payload[0];
} _packed_;

struct WakeupTime {
    uint32_t seconds;
} _packed_;

struct Serial {
    uint8_t serial[12];
} _packed_;

struct CommandWrite {
    uint8_t reg;
    union {
        Protocol protocol;
        Led led;
        FirmwareUpdate firmwareUpdate;
        WakeupTime wakeupTime;
        Serial serial;

        uint8_t unused[0];
    };
} _packed_;

} // namepsace FringProtocol

#endif // FRINGPROTOCOL_H
