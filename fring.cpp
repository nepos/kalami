#include <QtEndian>
#include <QDir>
#include <QThread>
#include <QTime>

#include "fring.h"
#include "gpio.h"
#include "crc32table.h"

Q_LOGGING_CATEGORY(FringLog, "Fring")

const int Fring::GPIONr = 8;
const int Fring::I2CBus = 0;
const int Fring::I2CAddr = 0x42;

Fring::Fring(QObject *parent) :
    QObject(parent),
    client(this),
    interruptGpio(Fring::GPIONr, this),
    updateThread(0),
    batteryLogFileName()
{
    interruptGpio.setEdge(GPIO::EdgeFalling);
    interruptGpio.setDirection(GPIO::DirectionIn);
    QObject::connect(&interruptGpio, &GPIO::onDataReady, this, &Fring::onInterrupt);

    homeButtonState = -1;
    batteryPresent = -1;
    ambientLightValue = -1;

    ledCacheValid = false;

    QByteArray batteryLogDir = qgetenv("KALAMI_BATTERY_LOG_DIR");
    if (!batteryLogDir.isEmpty()) {
        int i = 0;

        do {
            batteryLogFileName = QString(batteryLogDir) + "/batterylog-" + QString::number(++i) + ".log";
        } while(QFile::exists(batteryLogFileName));

        QFile f(batteryLogFileName);
        if (f.open(QFile::ReadWrite))
            qInfo(FringLog) << "Using" << batteryLogFileName << "to store battery logs";
        else
            batteryLogFileName.clear();
    }
}

bool Fring::initialize()
{
    if (!client.open(Fring::I2CBus, Fring::I2CAddr))
        return false;

    struct FringCommandRead rdCmd = {};
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_ID;
    wrCmd.protocolVersion.version = 1;
    if (!transfer(&wrCmd, offsetof(struct FringCommandWrite, protocolVersion) + sizeof(wrCmd.protocolVersion),
                  &rdCmd, sizeof(rdCmd.id)))
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
    QByteArray ba = QByteArray((char *) rdCmd.bootInfo.serial, sizeof(rdCmd.bootInfo.serial));

    // Check if the firmware has only 0xff in the serial number, and set a random serial in that case
    bool allFF = true;
    for (int i = 0; i < ba.size(); ++i)
        if (ba[i] != (char) 0xff) {
            allFF = false;
            break;
        }

    if (allFF) {
        for (int i = 0; i < ba.size(); ++i)
            ba[i] = (char) (qrand() & 0xff);

        wrCmd.reg = FRING_REG_SET_SERIAL;
        memcpy(&wrCmd.setSerial.serial, ba.constData(), sizeof(wrCmd.setSerial.serial));
        qInfo(FringLog) << "Setting firmware device serial:" << ba.toHex();
        transfer(&wrCmd, offsetof(struct FringCommandWrite, setSerial) + sizeof(wrCmd.setSerial.serial));
    }

    deviceSerial = QString(ba.toHex());

    wrCmd.reg = FRING_REG_READ_BOARD_REVISION;
    if (!transfer(&wrCmd, 1, &rdCmd, sizeof(rdCmd.boardRevision)))
        return false;

    boardRevisionA = rdCmd.boardRevision.boardRevisionA;
    boardRevisionB = rdCmd.boardRevision.boardRevisionB;

    qInfo(FringLog) << "Successfully initialized, firmware version" << firmwareVersion
                    << bootFlagsStrings.join(", ")
                    << "board revisions" << boardRevisionA << boardRevisionB;

    // Check for firmware updates
    QDir firmwareDir = QDir("/app/firmware/fring/");
    QStringList firmwareFiles = firmwareDir.entryList(QDir::Files);
    QString updateSuffix = (bootFlags & FRING_BOOT_STATUS_FIRMWARE_B) ? "bin-a" : "bin-b";

    foreach (QString firmwareFile, firmwareFiles) {
        QStringList parts = firmwareFile.split(".");

        if (!parts.isEmpty()) {
            QString suffix = parts.last();

            if (suffix != updateSuffix)
                continue;

            int availableVersion = parts.first().toInt();

            if (availableVersion > firmwareVersion) {
                qInfo(FringLog) << "Newer firmware available (" + firmwareFile + "). Starting update.";
                startFirmwareUpdate(firmwareDir.absolutePath() + "/" + firmwareFile);
            }
        }
    }

    return true;
}

const QString &Fring::getDeviceSerial()
{
    return deviceSerial;
}

bool Fring::transfer(const struct FringCommandWrite *wrCmd, size_t wrSize,
                     const struct FringCommandRead *rdCmd, size_t rdSize)
{
    struct FringCommandRead rdCmdDummy;

    // There's a bug in either the qcom i2c host driver or the firmware that causes
    // transfers to fail unless there is at least one byte read back in a 2nd command.
    if (rdCmd == NULL || rdSize == 0) {
        rdCmd = &rdCmdDummy;
        rdSize = 1;
    }

    if (!client.transfer((uint8_t *) wrCmd, wrSize, (uint8_t *) rdCmd, rdSize)) {
        qWarning(FringLog) << "Unable to transfer command!";
        return false;
    }

    return true;
}

bool Fring::setLed(const struct FringCommandWrite *wrCmd)
{
    int id = !!wrCmd->led.id;

    if (ledCacheValid && memcmp(&ledCache[id].led, &wrCmd->led, sizeof(wrCmd->led) == 0))
        return true;

    memcpy(&ledCache[id].led, &wrCmd->led, sizeof(wrCmd->led));
    ledCacheValid = true;

    return transfer(wrCmd, offsetof(struct FringCommandWrite, led) + sizeof(wrCmd->led));
}

bool Fring::setLedOff(int id)
{
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_SET_LED;
    wrCmd.led.id = id;
    wrCmd.led.mode = FRING_LED_MODE_OFF;

    return setLed(&wrCmd);
}

bool Fring::setLedOn(int id, float r, float g, float b)
{
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_SET_LED;
    wrCmd.led.id = id;
    wrCmd.led.mode = FRING_LED_MODE_ON;
    wrCmd.led.on.r = 255.0f * r;
    wrCmd.led.on.g = 255.0f * g;
    wrCmd.led.on.b = 255.0f * b;

    return setLed(&wrCmd);
}

bool Fring::setLedFlashing(int id, float r, float g, float b, float onPhase, float offPhase)
{
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_SET_LED;
    wrCmd.led.id = id;
    wrCmd.led.mode = FRING_LED_MODE_FLASHING;
    wrCmd.led.flashing.r = 255.0f * r;
    wrCmd.led.flashing.g = 255.0f * g;
    wrCmd.led.flashing.b = 255.0f * b;
    wrCmd.led.flashing.on = onPhase / 0.1f;
    wrCmd.led.flashing.off = offPhase / 0.1f;

    return setLed(&wrCmd);
}

bool Fring::setLedPulsating(int id, float r, float g, float b, float frequency)
{
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_SET_LED;
    wrCmd.led.id = id;
    wrCmd.led.mode = FRING_LED_MODE_PULSATING;
    wrCmd.led.pulsating.r = 255.0f * r;
    wrCmd.led.pulsating.g = 255.0f * g;
    wrCmd.led.pulsating.b = 255.0f * b;
    wrCmd.led.pulsating.period = 10.0f / frequency;

    return setLed(&wrCmd);
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
    bool battery = !!(status & FRING_DEVICE_STATUS_BATTERY_PRESENT);

    if (homeButtonState != home) {
        homeButtonState = !!home;
        emit homeButtonChanged(homeButtonState);
    }

    batteryPresent = !!battery;

    if (ambientLightValue == -1 ||
            ambientLightValue != rdCmd.deviceStatus.ambientLightValue) {
        ambientLightValue = rdCmd.deviceStatus.ambientLightValue;
        emit ambientLightChanged(ambientLightValue / 255.0);
    }

    hardwareErrors = qFromLittleEndian(rdCmd.deviceStatus.hardwareErrors);
    if (hardwareErrors)
        qWarning(FringLog) << "Detected hardware errors: " << QString::number(hardwareErrors, 16);

    return true;
}

bool Fring::readBatteryStatus()
{
    struct FringCommandRead rdCmd = {};
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_READ_BATTERY_STATUS;
    if (!transfer(&wrCmd, 1, &rdCmd, sizeof(rdCmd.batteryStatus)))
        return false;

    qInfo(FringLog) << "Battery status upate:";
    qInfo(FringLog) << QString::asprintf("Charge current %.2f A", rdCmd.batteryStatus.chargeCurrent * 0.05f);
    qInfo(FringLog) << QString::asprintf("Level %d%%", rdCmd.batteryStatus.level);
    qInfo(FringLog) << QString::asprintf("Temperature %d degree celcius", rdCmd.batteryStatus.temp);
    qInfo(FringLog) << QString::asprintf("Remaining capacity %d mAh", rdCmd.batteryStatus.remainingCapacity);
    qInfo(FringLog) << QString::asprintf("Average time to full %d min, to empty %d min",
                                         rdCmd.batteryStatus.averageTimeToFull, rdCmd.batteryStatus.averageTimeToEmpty);
    qInfo(FringLog) << QString::asprintf("Status 0x%02x", rdCmd.batteryStatus.status);

    if (batteryLevel != rdCmd.batteryStatus.level ||
            batteryChargeCurrent != rdCmd.batteryStatus.chargeCurrent) {
        batteryLevel = rdCmd.batteryStatus.level;
        batteryChargeCurrent = rdCmd.batteryStatus.chargeCurrent;

        emit batteryStateChanged(255.0f / (float) batteryLevel, (float) batteryChargeCurrent * 0.05f);
    }

    if (!batteryLogFileName.isEmpty()) {
        QFile f(batteryLogFileName);
        if (f.open(QFile::WriteOnly | QFile::Append | QFile::Unbuffered)) {
            static QTime *time = NULL;

            if (!time) {
                time = new QTime();
                time->start();
            }

            QStringList l;
            l << QString::number(time->elapsed());
            l << QString::number((float) rdCmd.batteryStatus.chargeCurrent * 0.05f);
            l << QString::number(rdCmd.batteryStatus.level);
            l << QString::number(rdCmd.batteryStatus.temp);
            l << QString::number(rdCmd.batteryStatus.remainingCapacity);
            l << QString::number(rdCmd.batteryStatus.averageTimeToFull);
            l << QString::number(rdCmd.batteryStatus.averageTimeToEmpty);
            l << QString::number(rdCmd.batteryStatus.status, 16);

            QTextStream stream(&f);
            stream << l.join(";") << endl;
            f.close();
        } else {
            qWarning(FringLog) << "Unable to write" << batteryLogFileName;
        }
    }

    return true;
}

bool Fring::readLogMessage()
{
    struct FringCommandWrite wrCmd = {};
    char buf[16];

    wrCmd.reg = FRING_REG_READ_LOG_MESSAGE;

    if (!client.transfer((uint8_t *) &wrCmd, 1, (uint8_t *) buf, sizeof(buf))) {
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

    if (status & FRING_INTERRUPT_BATTERY_STATUS)
        readBatteryStatus();

    if (status & FRING_INTERRUPT_LOG_MESSAGE)
        readLogMessage();

    if (status & FRING_INTERRUPT_FIRMWARE_UPDATE) {
        if (updateThread)
            updateThread->interrupt();
        else
            qWarning(FringLog) << "Firmware update interrupt with no update in progress? Uh-oh.";
    }
}

void Fring::startFirmwareUpdate(const QString filename)
{
    if (updateThread) {
        updateThread->quit();
        updateThread->deleteLater();
        updateThread = NULL;
    }

    updateThread = new FringUpdateThread(this, filename);

    QObject::connect(updateThread, &FringUpdateThread::succeeded, [this]() {
        qInfo(FringLog) << "Update thread succeeded.";
    });

    QObject::connect(updateThread, &FringUpdateThread::failed, [this]() {
        qInfo(FringLog) << "Update thread failed.";
    });

    QObject::connect(updateThread, &FringUpdateThread::progress, [this](float v) {
        qInfo(FringLog) << "Update thread progress:" << v;
    });

    updateThread->start();
}

FringUpdateThread::FringUpdateThread(Fring *fring, const QString &filename) : fring(fring), file(filename), semaphore()
{
}

FringUpdateThread::~FringUpdateThread()
{
    if (file.isOpen())
        file.close();
}

uint32_t FringUpdateThread::calculateCRC(uint32_t crc, const char *buf, size_t len)
{
    // STM32 implements CRC32-MPEG2 which uses big endian and no final flip mask

    while (len > 0) {
        crc = (crc << 8) ^ crc32table[((crc >> 24) ^ buf[3]) & 0xff];
        crc = (crc << 8) ^ crc32table[((crc >> 24) ^ buf[2]) & 0xff];
        crc = (crc << 8) ^ crc32table[((crc >> 24) ^ buf[1]) & 0xff];
        crc = (crc << 8) ^ crc32table[((crc >> 24) ^ buf[0]) & 0xff];

        buf += 4;
        len -= 4;
    }

    return crc;
}

void FringUpdateThread::run()
{
    uint32_t crc = ~0U;
    uint32_t offset = 0;
    qint64 r;
    static const size_t maxChunkSize = 32;
    struct FringCommandWrite *wrCmd;

    size_t wrSize =
            offsetof(struct FringCommandWrite, firmwareUpdate)
            + sizeof(wrCmd->firmwareUpdate)
            + maxChunkSize;
    wrCmd = (struct FringCommandWrite *) alloca(wrSize);

    if (!file.open(QFile::ReadOnly)) {
        qWarning(FringLog()) << "Unable to open file" << file.fileName();
        emit failed();
        return;
    }

    wrCmd->reg = FRING_REG_PUSH_FIRMWARE_UPDATE;

    qInfo(FringLog) << "Transmitting firmware file" << file.fileName() << "size" << file.size();

    do {
        r = file.read(wrCmd->firmwareUpdate.payload, maxChunkSize);
        if (r < 0) {
            qWarning(FringLog) << "Unable to read firmware file!";
            break;
        }

        // Align chunk size to 4 bytes
        r += 3;
        r &= ~3;

        crc = calculateCRC(crc, wrCmd->firmwareUpdate.payload, r);
        wrCmd->firmwareUpdate.crc = qToLittleEndian(crc);
        wrCmd->firmwareUpdate.length = qToLittleEndian(r);
        wrCmd->firmwareUpdate.offset = qToLittleEndian(offset);

        QString str;
        str.sprintf("Transmitting %lld bytes, offset %d, crc %08x", r, offset, crc);
        qInfo(FringLog) << str;

        if (!fring->transfer(wrCmd, wrSize)) {
            emit failed();
            return;
        }

        offset += r;

        // Wait for interrupt
        semaphore.acquire();

        if (interruptStatus != FRING_UPDATE_RESULT_OK) {
            qWarning(FringLog) << "Firmware returned bad code in response to update command:" << interruptStatus;
            emit failed();
            return;
        }

        emit progress((float) offset / (float) file.size());
    } while(r > 0);

    emit succeeded();
}

void FringUpdateThread::interrupt()
{
    struct FringCommandRead rdCmd = {};
    struct FringCommandWrite wrCmd = {};

    wrCmd.reg = FRING_REG_READ_FIRMWARE_UPDATE_RESULT;
    if (!fring->transfer(&wrCmd, 1, &rdCmd, sizeof(rdCmd.updateStatus)))
        return;

    interruptStatus = qFromLittleEndian(rdCmd.updateStatus.status);
    semaphore.release();
}
