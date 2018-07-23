#pragma once

#include <QObject>
#include <QtCore/QLoggingCategory>
#include <QNdefMessage>

Q_DECLARE_LOGGING_CATEGORY(NfcLog)

class QUrl;
class QPixmap;
class QNearFieldManager;
class QNearFieldTarget;
class QNearFieldTarget;

class Nfc : public QObject
{
    Q_OBJECT
public:
    explicit Nfc(QObject *parent = 0);

signals:
    void tagDetected(const QJsonObject &obj);

public slots:
    void targetDetected(QNearFieldTarget *target);
    void targetLost(QNearFieldTarget *target);
    void handleMessage(QNdefMessage message, QNearFieldTarget *target);
    bool setPollingEnabled(bool enabled);

private:
    QNearFieldManager *manager;
    bool pollingEnabled;
};
