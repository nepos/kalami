#include "connmanagent.h"

ConnmanAgent::ConnmanAgent(QObject *parent) :
    QObject(parent), adaptor(this)
{
}

void ConnmanAgent::Cancel()
{
    qInfo() << "CANCEL";
}

void ConnmanAgent::Release()
{

}

void ConnmanAgent::ReportError(const QDBusObjectPath &in0, const QString &in1)
{

}

void ConnmanAgent::RequestBrowser(const QDBusObjectPath &in0, const QString &in1)
{

}

QVariantMap ConnmanAgent::RequestInput(const QDBusObjectPath &in0, const QVariantMap &in1)
{

}
