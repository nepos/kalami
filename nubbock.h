#pragma once

#include <QDebug>
#include <QObject>
#include <QLocalSocket>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(NubbockLog)

class Nubbock : public QObject
{
    Q_OBJECT
public:
    explicit Nubbock(QObject *parent = nullptr);

public slots:
    void setTransform(const QString &t);

private:
    QString endpoint;
    QString transform;
    QLocalSocket socket;
    bool sendState(void);
};
