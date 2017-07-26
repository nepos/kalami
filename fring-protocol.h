#ifndef FRINGPROTOCOL_H
#define FRINGPROTOCOL_H

enum {
    FRING_REG_ID                    = 0x01,
    FRING_REG_READ_BOOT_INFO        = 0x02,
    FRING_REG_READ_BOARD_REVISION   = 0x03,
    FRING_REG_READ_INTERRUPT_STATUS = 0x04,
    FRING_REG_SET_LED               = 0x05,
    FRING_REG_READ_DEVICE_STATUS    = 0x06,
    FRING_REG_PUSH_FIRMWARE_UPDATE  = 0x07,
    FRING_REG_READ_LOG_MESSAGE      = 0x08,
};

enum {
    FRING_INTERRUPT_DEVICE_STATUS   = 0x01,
    FRING_INTERRUPT_LOG_MESSAGE     = 0x02
};

enum {
    FRING_LED_MODE_OFF              = 0,
    FRING_LED_MODE_ON               = 1,
    FRING_LED_MODE_FLASHING         = 2,
    FRING_LED_MODE_PULSATING        = 3
};

enum {
    FRING_UPDATE_STATUS_OK          = 0
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
            uint8_t bootedFirmware;
        } version _packed_;

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

        uint8_t unused[0];
    };
} _packed_;


#endif // FRINGPROTOCOL_H
