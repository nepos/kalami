#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <QObject>

#include "types.h"
#include "dbus_fi.w1.wpa_supplicant1.h"
#include "dbus_fi.w1.wpa_supplicant1.Interface.h"
#include "dbus_fi.w1.wpa_supplicant1.Network.h"
#include "dbus_fi.w1.wpa_supplicant1.BSS.h"

class WifiManager : public QObject
{
    Q_OBJECT
public:
    explicit WifiManager(QObject *parent = 0);
    ~WifiManager();

signals:

public slots:

private:
    QString statusString;
    FiW1Wpa_supplicant1Interface *wpaSupplicant;
    QList<FiW1Wpa_supplicant1InterfaceInterface*> interfaces;

    void scan();
    void monitorInterface(FiW1Wpa_supplicant1InterfaceInterface *interface);
};

#endif // WIFIMANAGER_H
