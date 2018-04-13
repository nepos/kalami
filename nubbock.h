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

    enum Transform {
        TRANSFORM_90,
        TRANSFORM_270,
    };

public slots:
    bool setTransform(enum Transform t);
    void suspend();
    void resume();

private:
    QString endpoint;
    enum Transform transform;
    bool suspended;
    QLocalSocket socket;
    bool sendState(void);
};
