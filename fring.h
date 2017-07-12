#pragma once

#include <QObject>
#include "i2cclient.h"
#include "gpio.h"

Q_DECLARE_LOGGING_CATEGORY(FringLog)

struct FringCommandRead;
struct FringCommandWrite;

class Fring : public QObject
{
    Q_OBJECT
public:
    explicit Fring(QObject *parent = 0);
    bool initialize();

signals:
    void homeButtonChanged(bool state);
    void batteryStateChanged(float level, float chargeCurrent, float dischargeCurrent);

public slots:
    bool setLedOff(int id);
    bool setLedOn(int id, float r, float g, float b);
    bool setLedFlashing(int id, float r, float g, float b, float onPhase, float offPhase);
    bool setLedPulsating(int id, float r, float g, float b, float period);

private slots:
    void onInterrupt(GPIO::Value v);

private:
    static const int GPIONr;
    static const int I2CAddr;
    static const int I2CBus;

    I2CClient client;
    GPIO interruptGpio;

    int firmwareVersion;
    int boardRevisionA;
    int boardRevisionB;

    int homeButtonState;
    int batteryLevel;
    int batteryChargeCurrent;
    int batteryDischargeCurrent;

    bool transfer(const struct FringCommandWrite *wrCmd, const FringCommandRead *rdCmd = 0);
    bool readDeviceStatus();
};
