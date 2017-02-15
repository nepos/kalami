#include <QDebug>
#include "connman.h"
#include <qconnman/manager.h>

Connman::Connman(QObject *parent) : QObject(parent)
{
    Manager manager;

    qDebug() << " XXXXX >>> ";

    foreach (Service *service, manager.services())
        qDebug() << service;

    qDebug() << " XXXXX <<< ";
}
