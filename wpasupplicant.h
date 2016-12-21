#ifndef WPASUPPLICANT_H
#define WPASUPPLICANT_H

#include <QObject>
#include <QList>

#include <wpasupplicantinterface.h>

class WpaSupplicant : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList capabilities   READ getCapabilities                           NOTIFY capabilitiesChanged)
    Q_PROPERTY(QStringList eapMethods     READ getEapMethods                             NOTIFY eapMethodsChanged)
    Q_PROPERTY(QString     debugLevel     READ getDebugLevel     WRITE setDebugLevel     NOTIFY debugLevelChanged)
    Q_PROPERTY(bool        debugTimestamp READ getDebugTimestamp WRITE setDebugTimestamp NOTIFY debugTimestampChanged)
    Q_PROPERTY(bool        debugShowKeys  READ getDebugShowKeys  WRITE setDebugShowKeys  NOTIFY debugShowKeysChanged)

public:
    explicit WpaSupplicant(QObject *parent = 0);
    ~WpaSupplicant();

    const QStringList& getCapabilities() const { return capabilities; }
    const QStringList& getEapMethods() const { return eapMethods; }
    const QList<WpaSupplicantInterface *> getInterfaces() const { return interfaces; }

    const QString& getDebugLevel() const { return debugLevel; }
    void setDebugLevel(const QString &debugLevel);

    bool getDebugTimestamp() const { return debugTimestamp; }
    void setDebugTimestamp(bool debugTimestamp);

    bool getDebugShowKeys() const { return debugShowKeys; }
    void setDebugShowKeys(bool debugShowKeys);

signals:
    void capabilitiesChanged(const QStringList &capabilities);
    void eapMethodsChanged(const QStringList &eapMethods);
    void debugLevelChanged(const QString &debugLevel);
    void debugTimestampChanged(bool debugTimestamp);
    void debugShowKeysChanged(bool debugTimestamp);

    void interfaceAdded(const WpaSupplicantInterface &interface);
    void interfaceRemoved(const WpaSupplicantInterface &interface);

private slots:
    void readProperties(bool doEmit = true);

private:
    QDBusInterface *dbus;
    QList<WpaSupplicantInterface *> interfaces;

    QStringList capabilities;
    QStringList eapMethods;
    QString debugLevel;

    bool debugTimestamp;
    bool debugShowKeys;
};

#endif // WPASUPPLICANT_H
