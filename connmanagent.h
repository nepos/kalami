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
    void ReportError(const QDBusObjectPath &service, const QString &error);
    void RequestBrowser(const QDBusObjectPath &service, const QString &url);
    QVariantMap RequestInput(const QDBusObjectPath &servicePath, const QVariantMap &fields);

private:
    ConnmanAgentAdaptor adaptor;
};

#endif // CONNMANAGENT_H
