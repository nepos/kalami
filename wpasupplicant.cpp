#include <QDebug>
#include <QMetaMethod>
#include "wpasupplicant.h"

WpaSupplicant::WpaSupplicant(QObject *parent) :
    QObject(parent)
{
    dbus = new QDBusInterface("fi.w1.wpa_supplicant1",
                              "/fi/w1/wpa_supplicant1",
                              "fi.w1.wpa_supplicant1",
                              QDBusConnection::systemBus(), this);

    qDebug() << "SERVICE" << dbus->service() << "valid?" << dbus->isValid();

    // 0x617b73767d = "a{sv}"
    QObject::connect(dbus, SIGNAL(PropertiesChanged(QDBusRawType<0x617b73767d>*)), this, SLOT(readProperties()));

    QList<QDBusObjectPath> list = dbus->property("Interfaces").value<QList<QDBusObjectPath> >();
    for (int i = 0; i < list.size(); i++)
        interfaces.append(new WpaSupplicantInterface(list.at(i)));
        //emit interfaceAdded(*interface);

    readProperties(false);
}

void WpaSupplicant::readProperties(bool doEmit)
{
    // QStringList, "as"
    QStringList as;

    as = dbus->property("Capabilities").toStringList();
    if (doEmit && as != capabilities)
        emit capabilitiesChanged(as);

    capabilities = as;

    as = dbus->property("EapMethods").toStringList();
    if (doEmit && as != eapMethods)
        emit eapMethodsChanged(as);

    eapMethods = as;

    // QString, "s"
    QString s;

    s = dbus->property("DebugLevel").toString();
    if (doEmit && s != debugLevel)
        emit debugLevelChanged(s);

    debugLevel = s;

    // bool, "b"
    bool b;

    b = dbus->property("DebugTimestamp").toBool();
    if (doEmit && b != debugTimestamp)
        emit debugTimestampChanged(b);

    debugTimestamp = b;

    b = dbus->property("DebugShowKeys").toBool();
    if (doEmit && b != debugShowKeys)
        emit debugShowKeysChanged(b);

    debugShowKeys = b;
}

void WpaSupplicant::setDebugLevel(const QString &debugLevel)
{
    dbus->setProperty("DebugLevel", debugLevel);
}

void WpaSupplicant::setDebugTimestamp(bool debugTimestamp)
{
    dbus->setProperty("DebugTimestamp", debugTimestamp);
}

void WpaSupplicant::setDebugShowKeys(bool debugShowKeys)
{
    dbus->setProperty("DebugShowKeys", debugShowKeys);
}

WpaSupplicant::~WpaSupplicant()
{
    while (!interfaces.isEmpty())
        delete interfaces.takeFirst();

    delete dbus;
}
