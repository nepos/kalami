#ifndef FRINGPROTOCOL_H
#define FRINGPROTOCOL_H

enum {
    FRING_REG_ID                          = 0x01,
    FRING_REG_READ_BOOT_INFO              = 0x02,
    FRING_REG_READ_BOARD_REVISION         = 0x03,
    FRING_REG_READ_INTERRUPT_STATUS       = 0x04,
    FRING_REG_SET_LED                     = 0x05,
    FRING_REG_READ_DEVICE_STATUS          = 0x06,
    FRING_REG_PUSH_FIRMWARE_UPDATE        = 0x07,
    FRING_REG_READ_FIRMWARE_UPDATE_RESULT = 0x08,
    FRING_REG_READ_LOG_MESSAGE            = 0x09,
    FRING_REG_SET_SERIAL                  = 0x0a,
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
    FRING_DEVICE_STATUS_HOME_BUTTON     = 0x01,
    FRING_DEVICE_STATUS_BATTERY_PRESENT = 0x02,
};

enum {
    FRING_INTERRUPT_DEVICE_STATUS   = 0x01,
    FRING_INTERRUPT_LOG_MESSAGE     = 0x02,
    FRING_INTERRUPT_FIRMWARE_UPDATE = 0x04
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

struct FringCommandRead {
    union {
        struct {
            uint8_t id[5];
        } id;

        struct {
            uint32_t version;
            uint32_t uptime;
            uint8_t serial[12];
            uint32_t flags;
        } bootInfo _packed_;

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
            uint8_t ambientLightValue;
            uint32_t status;
            uint32_t hardwareErrors;
        } deviceStatus;

        struct {
            uint32_t status;
        } updateStatus;

        uint8_t unused[0];
    };
}  _packed_;

struct FringCommandWrite {
    uint8_t reg;
    union {
        struct {
            uint8_t version;
        } protocolVersion;

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

        struct {
            uint32_t length;
            uint32_t offset;
            uint32_t crc;
            char payload[0];
        } firmwareUpdate;

        struct {
            uint8_t serial[12];
        } setSerial;

        uint8_t unused[0];
    };
} _packed_;


#endif // FRINGPROTOCOL_H
