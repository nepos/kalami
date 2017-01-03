#ifndef CONNMANAGENT_H
#define CONNMANAGENT_H

#include <QObject>
#include "connmanagentadaptor.h"

class ConnmanAgent : public QObject
{
    Q_OBJECT
public:
    explicit ConnmanAgent(QObject *parent = 0);

signals:

public slots:
    void Cancel();
    void Release();
    void ReportError(const QDBusObjectPath &in0, const QString &in1);
    void RequestBrowser(const QDBusObjectPath &in0, const QString &in1);
    QVariantMap RequestInput(const QDBusObjectPath &in0, const QVariantMap &in1);

private:
    ConnmanAgentAdaptor adaptor;
};

#endif // CONNMANAGENT_H
