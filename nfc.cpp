/***
  Copyright (c) 2017 Nepos GmbH

  Authors: Pascal Huerst <pascal.huerst@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
***/

#include <QDebug>
#include <QNearFieldManager>
#include <QNdefMessage>

#include "nfc.h"

Q_LOGGING_CATEGORY(NfcLog, "Nfc")

Nfc::Nfc(QObject *parent) :
    QObject(parent),
    manager(new QNearFieldManager(this))
{
    if (!manager->isAvailable()) {
        qWarning(NfcLog) << "NFC not available";
        return;
    }

    QNdefFilter filter;

    //filter.setOrderMatch(false);
    //filter.appendRecord<QNdefNfcTextRecord>(1, UINT_MAX);
    //filter.appendRecord<QNdefNfcUriRecord>();
    // type parameter cannot specify substring so filter for "image/" below
    //filter.appendRecord(QNdefRecord::Mime, QByteArray(), 0, 1);

    int result = manager->registerNdefMessageHandler(filter, this,
                                                     SLOT(handleMessage(QNdefMessage, QNearFieldTarget*)));

    if (result < 0)
        qWarning(NfcLog) << "Platform does not support NDEF message handler registration";

    if (!manager->startTargetDetection()) {
        qWarning(NfcLog) << "Can not start target Detection";
    }

    connect(manager, SIGNAL(targetDetected(QNearFieldTarget*)),
            this, SLOT(targetDetected(QNearFieldTarget*)));
    connect(manager, SIGNAL(targetLost(QNearFieldTarget*)),
            this, SLOT(targetLost(QNearFieldTarget*)));
}


void Nfc::targetDetected(QNearFieldTarget *target)
{
    if (!target)
        return;

    qInfo(NfcLog) << "Found Tag: " << target->uid();

    connect(target, SIGNAL(ndefMessageRead(QNdefMessage)), this, SLOT(handlePolledNdefMessage(QNdefMessage)));
    target->readNdefMessages();
}

void Nfc::targetLost(QNearFieldTarget *target)
{
    if (target) {
        qInfo(NfcLog) << "Lost Tag: " << target->uid();

        target->deleteLater();
    }
}

void Nfc::handlePolledNdefMessage(QNdefMessage message)
{
    qInfo(NfcLog) << "Message Received: " << message.toByteArray();
}

void Nfc::handleMessage(QNdefMessage message, QNearFieldTarget *target)
{
    qInfo(NfcLog) << "Message Received: " << message.toByteArray();

    if (target)
        qInfo(NfcLog) << "handle Message Tag: " << target->uid();
}
