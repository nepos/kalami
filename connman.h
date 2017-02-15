#ifndef CONNMAN_H
#define CONNMAN_H

#include <QObject>

class Connman : public QObject
{
    Q_OBJECT
public:
    explicit Connman(QObject *parent = 0);

signals:

public slots:
};

#endif // CONNMAN_H