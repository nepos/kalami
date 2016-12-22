/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: 
 *
 * qdbusxml2cpp is Copyright (C) 2016 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef DBUS_FI_W1_WPA_SUPPLICANT1_INTERFACE_H
#define DBUS_FI_W1_WPA_SUPPLICANT1_INTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

#include "types.h"

/*
 * Proxy class for interface fi.w1.wpa_supplicant1.Interface
 */
class FiW1Wpa_supplicant1InterfaceInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "fi.w1.wpa_supplicant1.Interface"; }

public:
    FiW1Wpa_supplicant1InterfaceInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~FiW1Wpa_supplicant1InterfaceInterface();

    Q_PROPERTY(uint ApScan READ apScan WRITE setApScan)
    inline uint apScan() const
    { return qvariant_cast< uint >(property("ApScan")); }
    inline void setApScan(uint value)
    { setProperty("ApScan", QVariant::fromValue(value)); }

    Q_PROPERTY(uint BSSExpireAge READ bSSExpireAge WRITE setBSSExpireAge)
    inline uint bSSExpireAge() const
    { return qvariant_cast< uint >(property("BSSExpireAge")); }
    inline void setBSSExpireAge(uint value)
    { setProperty("BSSExpireAge", QVariant::fromValue(value)); }

    Q_PROPERTY(uint BSSExpireCount READ bSSExpireCount WRITE setBSSExpireCount)
    inline uint bSSExpireCount() const
    { return qvariant_cast< uint >(property("BSSExpireCount")); }
    inline void setBSSExpireCount(uint value)
    { setProperty("BSSExpireCount", QVariant::fromValue(value)); }

    Q_PROPERTY(QList<QDBusObjectPath> BSSs READ bSSs)
    inline QList<QDBusObjectPath> bSSs() const
    { return qvariant_cast< QList<QDBusObjectPath> >(property("BSSs")); }

    Q_PROPERTY(StringByteArrayMap Blobs READ blobs)
    inline StringByteArrayMap blobs() const
    { return qvariant_cast< StringByteArrayMap >(property("Blobs")); }

    Q_PROPERTY(QString BridgeIfname READ bridgeIfname)
    inline QString bridgeIfname() const
    { return qvariant_cast< QString >(property("BridgeIfname")); }

    Q_PROPERTY(StringVariantMap Capabilities READ capabilities)
    inline StringVariantMap capabilities() const
    { return qvariant_cast< StringVariantMap >(property("Capabilities")); }

    Q_PROPERTY(QString Country READ country WRITE setCountry)
    inline QString country() const
    { return qvariant_cast< QString >(property("Country")); }
    inline void setCountry(const QString &value)
    { setProperty("Country", QVariant::fromValue(value)); }

    Q_PROPERTY(QString CurrentAuthMode READ currentAuthMode)
    inline QString currentAuthMode() const
    { return qvariant_cast< QString >(property("CurrentAuthMode")); }

    Q_PROPERTY(QDBusObjectPath CurrentBSS READ currentBSS)
    inline QDBusObjectPath currentBSS() const
    { return qvariant_cast< QDBusObjectPath >(property("CurrentBSS")); }

    Q_PROPERTY(QDBusObjectPath CurrentNetwork READ currentNetwork)
    inline QDBusObjectPath currentNetwork() const
    { return qvariant_cast< QDBusObjectPath >(property("CurrentNetwork")); }

    Q_PROPERTY(int DisconnectReason READ disconnectReason)
    inline int disconnectReason() const
    { return qvariant_cast< int >(property("DisconnectReason")); }

    Q_PROPERTY(QString Driver READ driver)
    inline QString driver() const
    { return qvariant_cast< QString >(property("Driver")); }

    Q_PROPERTY(bool FastReauth READ fastReauth WRITE setFastReauth)
    inline bool fastReauth() const
    { return qvariant_cast< bool >(property("FastReauth")); }
    inline void setFastReauth(bool value)
    { setProperty("FastReauth", QVariant::fromValue(value)); }

    Q_PROPERTY(QString Ifname READ ifname)
    inline QString ifname() const
    { return qvariant_cast< QString >(property("Ifname")); }

    Q_PROPERTY(QList<QDBusObjectPath> Networks READ networks)
    inline QList<QDBusObjectPath> networks() const
    { return qvariant_cast< QList<QDBusObjectPath> >(property("Networks")); }

    Q_PROPERTY(QString PKCS11EnginePath READ pKCS11EnginePath)
    inline QString pKCS11EnginePath() const
    { return qvariant_cast< QString >(property("PKCS11EnginePath")); }

    Q_PROPERTY(QString PKCS11ModulePath READ pKCS11ModulePath)
    inline QString pKCS11ModulePath() const
    { return qvariant_cast< QString >(property("PKCS11ModulePath")); }

    Q_PROPERTY(int ScanInterval READ scanInterval WRITE setScanInterval)
    inline int scanInterval() const
    { return qvariant_cast< int >(property("ScanInterval")); }
    inline void setScanInterval(int value)
    { setProperty("ScanInterval", QVariant::fromValue(value)); }

    Q_PROPERTY(bool Scanning READ scanning)
    inline bool scanning() const
    { return qvariant_cast< bool >(property("Scanning")); }

    Q_PROPERTY(QString State READ state)
    inline QString state() const
    { return qvariant_cast< QString >(property("State")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> AddBlob(const QString &name, const QByteArray &data)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(name) << QVariant::fromValue(data);
        return asyncCallWithArgumentList(QStringLiteral("AddBlob"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> AddNetwork(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("AddNetwork"), argumentList);
    }

    inline QDBusPendingReply<> Disconnect()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Disconnect"), argumentList);
    }

    inline QDBusPendingReply<> EAPLogoff()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("EAPLogoff"), argumentList);
    }

    inline QDBusPendingReply<> EAPLogon()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("EAPLogon"), argumentList);
    }

    inline QDBusPendingReply<> FlushBSS(uint age)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(age);
        return asyncCallWithArgumentList(QStringLiteral("FlushBSS"), argumentList);
    }

    inline QDBusPendingReply<QByteArray> GetBlob(const QString &name)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(name);
        return asyncCallWithArgumentList(QStringLiteral("GetBlob"), argumentList);
    }

    inline QDBusPendingReply<> NetworkReply(const QDBusObjectPath &path, const QString &field, const QString &value)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(path) << QVariant::fromValue(field) << QVariant::fromValue(value);
        return asyncCallWithArgumentList(QStringLiteral("NetworkReply"), argumentList);
    }

    inline QDBusPendingReply<> Reassociate()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Reassociate"), argumentList);
    }

    inline QDBusPendingReply<> Reattach()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Reattach"), argumentList);
    }

    inline QDBusPendingReply<> Reconnect()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Reconnect"), argumentList);
    }

    inline QDBusPendingReply<> RemoveAllNetworks()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("RemoveAllNetworks"), argumentList);
    }

    inline QDBusPendingReply<> RemoveBlob(const QString &name)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(name);
        return asyncCallWithArgumentList(QStringLiteral("RemoveBlob"), argumentList);
    }

    inline QDBusPendingReply<> RemoveNetwork(const QDBusObjectPath &path)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(path);
        return asyncCallWithArgumentList(QStringLiteral("RemoveNetwork"), argumentList);
    }

    inline QDBusPendingReply<> Scan(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("Scan"), argumentList);
    }

    inline QDBusPendingReply<> SelectNetwork(const QDBusObjectPath &path)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(path);
        return asyncCallWithArgumentList(QStringLiteral("SelectNetwork"), argumentList);
    }

    inline QDBusPendingReply<> SetPKCS11EngineAndModulePath(const QString &pkcs11_engine_path, const QString &pkcs11_module_path)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(pkcs11_engine_path) << QVariant::fromValue(pkcs11_module_path);
        return asyncCallWithArgumentList(QStringLiteral("SetPKCS11EngineAndModulePath"), argumentList);
    }

    inline QDBusPendingReply<StringVariantMap> SignalPoll()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("SignalPoll"), argumentList);
    }

    inline QDBusPendingReply<> SubscribeProbeReq()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("SubscribeProbeReq"), argumentList);
    }

    inline QDBusPendingReply<> UnsubscribeProbeReq()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("UnsubscribeProbeReq"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void BSSAdded(const QDBusObjectPath &path, StringVariantMap properties);
    void BSSRemoved(const QDBusObjectPath &path);
    void BlobAdded(const QString &name);
    void BlobRemoved(const QString &name);
    void Certification(StringVariantMap certification);
    void EAP(const QString &status, const QString &parameter);
    void NetworkAdded(const QDBusObjectPath &path, StringVariantMap properties);
    void NetworkRemoved(const QDBusObjectPath &path);
    void NetworkRequest(const QDBusObjectPath &path, const QString &field, const QString &text);
    void NetworkSelected(const QDBusObjectPath &path);
    void ProbeRequest(StringVariantMap args);
    void PropertiesChanged(StringVariantMap properties);
    void ScanDone(bool success);
    void StaAuthorized(const QString &name);
    void StaDeauthorized(const QString &name);
};

/*
 * Proxy class for interface fi.w1.wpa_supplicant1.Interface.P2PDevice
 */
class FiW1Wpa_supplicant1InterfaceP2PDeviceInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "fi.w1.wpa_supplicant1.Interface.P2PDevice"; }

public:
    FiW1Wpa_supplicant1InterfaceP2PDeviceInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~FiW1Wpa_supplicant1InterfaceP2PDeviceInterface();

    Q_PROPERTY(QDBusObjectPath Group READ group)
    inline QDBusObjectPath group() const
    { return qvariant_cast< QDBusObjectPath >(property("Group")); }

    Q_PROPERTY(StringVariantMap P2PDeviceConfig READ p2PDeviceConfig WRITE setP2PDeviceConfig)
    inline StringVariantMap p2PDeviceConfig() const
    { return qvariant_cast< StringVariantMap >(property("P2PDeviceConfig")); }
    inline void setP2PDeviceConfig(StringVariantMap value)
    { setProperty("P2PDeviceConfig", QVariant::fromValue(value)); }

    Q_PROPERTY(QDBusObjectPath PeerGO READ peerGO)
    inline QDBusObjectPath peerGO() const
    { return qvariant_cast< QDBusObjectPath >(property("PeerGO")); }

    Q_PROPERTY(QList<QDBusObjectPath> Peers READ peers)
    inline QList<QDBusObjectPath> peers() const
    { return qvariant_cast< QList<QDBusObjectPath> >(property("Peers")); }

    Q_PROPERTY(QList<QDBusObjectPath> PersistentGroups READ persistentGroups)
    inline QList<QDBusObjectPath> persistentGroups() const
    { return qvariant_cast< QList<QDBusObjectPath> >(property("PersistentGroups")); }

    Q_PROPERTY(QString Role READ role)
    inline QString role() const
    { return qvariant_cast< QString >(property("Role")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> AddPersistentGroup(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("AddPersistentGroup"), argumentList);
    }

    inline QDBusPendingReply<> AddService(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("AddService"), argumentList);
    }

    inline QDBusPendingReply<> Cancel()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Cancel"), argumentList);
    }

    inline QDBusPendingReply<QString> Connect(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("Connect"), argumentList);
    }

    inline QDBusPendingReply<> DeleteService(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("DeleteService"), argumentList);
    }

    inline QDBusPendingReply<> Disconnect()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Disconnect"), argumentList);
    }

    inline QDBusPendingReply<> ExtendedListen(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("ExtendedListen"), argumentList);
    }

    inline QDBusPendingReply<> Find(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("Find"), argumentList);
    }

    inline QDBusPendingReply<> Flush()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Flush"), argumentList);
    }

    inline QDBusPendingReply<> FlushService()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("FlushService"), argumentList);
    }

    inline QDBusPendingReply<> GroupAdd(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("GroupAdd"), argumentList);
    }

    inline QDBusPendingReply<> Invite(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("Invite"), argumentList);
    }

    inline QDBusPendingReply<> Listen(int timeout)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(timeout);
        return asyncCallWithArgumentList(QStringLiteral("Listen"), argumentList);
    }

    inline QDBusPendingReply<> PresenceRequest(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("PresenceRequest"), argumentList);
    }

    inline QDBusPendingReply<> ProvisionDiscoveryRequest(const QDBusObjectPath &peer, const QString &config_method)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(peer) << QVariant::fromValue(config_method);
        return asyncCallWithArgumentList(QStringLiteral("ProvisionDiscoveryRequest"), argumentList);
    }

    inline QDBusPendingReply<> RejectPeer(const QDBusObjectPath &peer)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(peer);
        return asyncCallWithArgumentList(QStringLiteral("RejectPeer"), argumentList);
    }

    inline QDBusPendingReply<> RemoveAllPersistentGroups()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("RemoveAllPersistentGroups"), argumentList);
    }

    inline QDBusPendingReply<> RemoveClient(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("RemoveClient"), argumentList);
    }

    inline QDBusPendingReply<> RemovePersistentGroup(const QDBusObjectPath &path)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(path);
        return asyncCallWithArgumentList(QStringLiteral("RemovePersistentGroup"), argumentList);
    }

    inline QDBusPendingReply<> ServiceDiscoveryCancelRequest(qulonglong args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("ServiceDiscoveryCancelRequest"), argumentList);
    }

    inline QDBusPendingReply<> ServiceDiscoveryExternal(int arg)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(arg);
        return asyncCallWithArgumentList(QStringLiteral("ServiceDiscoveryExternal"), argumentList);
    }

    inline QDBusPendingReply<qulonglong> ServiceDiscoveryRequest(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("ServiceDiscoveryRequest"), argumentList);
    }

    inline QDBusPendingReply<> ServiceDiscoveryResponse(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("ServiceDiscoveryResponse"), argumentList);
    }

    inline QDBusPendingReply<> ServiceUpdate()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("ServiceUpdate"), argumentList);
    }

    inline QDBusPendingReply<> StopFind()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("StopFind"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void DeviceFound(const QDBusObjectPath &path);
    void DeviceLost(const QDBusObjectPath &path);
    void FindStopped();
    void GONegotiationFailure(StringVariantMap properties);
    void GONegotiationRequest(const QDBusObjectPath &path, ushort dev_passwd_id, uchar device_go_intent);
    void GONegotiationSuccess(StringVariantMap properties);
    void GroupFinished(StringVariantMap properties);
    void GroupFormationFailure(const QString &reason);
    void GroupStarted(StringVariantMap properties);
    void InvitationReceived(StringVariantMap properties);
    void InvitationResult(StringVariantMap invite_result);
    void PersistentGroupAdded(const QDBusObjectPath &path, StringVariantMap properties);
    void PersistentGroupRemoved(const QDBusObjectPath &path);
    void ProvisionDiscoveryFailure(const QDBusObjectPath &peer_object, int status);
    void ProvisionDiscoveryPBCRequest(const QDBusObjectPath &peer_object);
    void ProvisionDiscoveryPBCResponse(const QDBusObjectPath &peer_object);
    void ProvisionDiscoveryRequestDisplayPin(const QDBusObjectPath &peer_object, const QString &pin);
    void ProvisionDiscoveryRequestEnterPin(const QDBusObjectPath &peer_object);
    void ProvisionDiscoveryResponseDisplayPin(const QDBusObjectPath &peer_object, const QString &pin);
    void ProvisionDiscoveryResponseEnterPin(const QDBusObjectPath &peer_object);
//    void ServiceDiscoveryRequest(StringVariantMap sd_request);
//    void ServiceDiscoveryResponse(StringVariantMap sd_response);
    void WpsFailed(const QString &name, StringVariantMap args);
};

/*
 * Proxy class for interface fi.w1.wpa_supplicant1.Interface.WPS
 */
class FiW1Wpa_supplicant1InterfaceWPSInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "fi.w1.wpa_supplicant1.Interface.WPS"; }

public:
    FiW1Wpa_supplicant1InterfaceWPSInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~FiW1Wpa_supplicant1InterfaceWPSInterface();

    Q_PROPERTY(QString ConfigMethods READ configMethods WRITE setConfigMethods)
    inline QString configMethods() const
    { return qvariant_cast< QString >(property("ConfigMethods")); }
    inline void setConfigMethods(const QString &value)
    { setProperty("ConfigMethods", QVariant::fromValue(value)); }

    Q_PROPERTY(bool ProcessCredentials READ processCredentials WRITE setProcessCredentials)
    inline bool processCredentials() const
    { return qvariant_cast< bool >(property("ProcessCredentials")); }
    inline void setProcessCredentials(bool value)
    { setProperty("ProcessCredentials", QVariant::fromValue(value)); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> Cancel()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Cancel"), argumentList);
    }

    inline QDBusPendingReply<StringVariantMap> Start(StringVariantMap args)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(args);
        return asyncCallWithArgumentList(QStringLiteral("Start"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void Credentials(StringVariantMap credentials);
    void Event(const QString &name, StringVariantMap args);
    void PropertiesChanged(StringVariantMap properties);
};

namespace fi {
  namespace w1 {
    namespace wpa_supplicant1 {
      //typedef ::FiW1Wpa_supplicant1InterfaceInterface Interface;
      namespace Interface {
        typedef ::FiW1Wpa_supplicant1InterfaceP2PDeviceInterface P2PDevice;
        typedef ::FiW1Wpa_supplicant1InterfaceWPSInterface WPS;
      }
    }
  }
}
#endif
