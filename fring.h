#pragma once

#include <QObject>
#include <QSemaphore>
#include <QThread>
#include "i2cclient.h"
#include "gpio.h"
#include "fring-protocol.h"

Q_DECLARE_LOGGING_CATEGORY(FringLog)

struct FringCommandRead;
struct FringCommandWrite;
class FringUpdateThread;

class Fring : public QObject
{
    Q_OBJECT
public:
    explicit Fring(QObject *parent = 0);
    bool initialize();
    const QString &getDeviceSerial();

    int getFirmwareVersion() const { return firmwareVersion; }
    uint32_t getHardwareErrors() const { return hardwareErrors; }
    int getBoardRevisionA() const { return boardRevisionA; }
    int getBoardRevisionB() const { return boardRevisionB; }

    enum WakeupReason {
        WAKEUP_REASON_NONE = 1,
        WAKEUP_REASON_HOMEBUTTON,
        WAKEUP_REASON_RTC
    };

signals:
    void homeButtonChanged(bool state);
    void ambientLightChanged(double value);
    void batteryStateChanged(double level, double chargeCurrent, double temperature, double timeToEmpty, double timeToFull);
    void logMessageReceived(const QString &message);
    void wakeupReasonChanged(WakeupReason w);
    void hardwareErrorsChanged();

public slots:
    bool setLedOff(int id);
    bool setLedOn(int id, double r, double g, double b);
    bool setLedFlashing(int id, double r, double g, double b, double onPhase, double offPhase);
    bool setLedPulsating(int id, double r, double g, double b, double frequency);
    void startFirmwareUpdate(const QString filename);
    void setWakeupMs(uint32_t ms);

private slots:
    bool setLed(const FringProtocol::CommandWrite *wrCmd);
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
    QString deviceSerial;

    int homeButtonState;
    int batteryPresent;
    int batteryLevel;
    int batteryChargeCurrent;
    int batteryTemperature;
    int batteryTimeToEmpty;
    int batteryTimeToFull;

    int ambientLightValue;
    uint32_t hardwareErrors;

    bool readDeviceStatus();
    bool readBatteryStatus();
    bool readLogMessage();
    bool readWakeupReason();
    uint32_t calculateCRC(uint32_t crc, const char *buf, size_t len);

    FringUpdateThread *updateThread;

    QString batteryLogFileName;

    FringProtocol::CommandWrite ledCache[2];
    bool ledCacheValid;

protected:
    friend class FringUpdateThread;
    bool transfer(const FringProtocol::CommandWrite *wrCmd, size_t wrSize, const FringProtocol::CommandRead *rdCmd = 0, size_t rdSize = 0);
};

class FringUpdateThread : public QThread
{
    Q_OBJECT

public:
    FringUpdateThread(Fring *fring, const QString &filename);
    void run() Q_DECL_OVERRIDE;
    ~FringUpdateThread();

public slots:
    void interrupt();

signals:
    void progress(double progress);
    void succeeded();
    void failed();

private:
    int interruptStatus;
    double lastEmittedProgress;
    Fring *fring;
    QFile file;
    QSemaphore semaphore;
    uint32_t calculateCRC(uint32_t crc, const char *buf, size_t len);

    void emitProgress(double v);
};
