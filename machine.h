#ifndef MACHINE_H
#define MACHINE_H

#include <QObject>

class Machine : public QObject
{
    Q_OBJECT
public:
    explicit Machine(QObject *parent = 0);

signals:

public slots:
};

#endif // MACHINE_H