/***
  Copyright (c) 2017 Nepos GmbH

  Authors: Daniel Mack <daniel@nepos.io>

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

#include <QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>

#include "daemon.h"
#include "machine.h"
#include "updater.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;

    parser.setApplicationDescription("Hardware abstraction layer for Nepos embedded devices");
    parser.addHelpOption();

    QCommandLineOption serverOption(QStringList() <<
                                    "s" << "server",
                                    "Websocket URI to connect to",
                                    QStringLiteral("server"), QStringLiteral("ws://localhost:3000/ws/kalami"));
    parser.addOption(serverOption);
    parser.process(app);

    Daemon d(QUrl(parser.value(serverOption)), &app);

    return app.exec();
}
