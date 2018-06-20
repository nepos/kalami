/***
  Copyright (c) 2017,2018 Nepos GmbH

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

#define NORMAL  "\033[0m"

#define RED_HI  "\033[1;31m"
#define RED_LO  "\033[0;31m"

#define BLUE_HI  "\033[1;34m"
#define BLUE_LO  "\033[0;34m"

#define GRAY_HI  "\033[1;37m"
#define GRAY_LO  "\033[0;37m"

#define YELLOW_HI "\033[1;33m"
#define YELLOW_LO "\033[0;33m"


void kalamiMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    /*
     * Kalami Log Levels:
     *
     * 0 -> Fatal (Red)
     * 1 -> Critical (Red)
     * 2 -> Warning (Blue)
     * 3 -> Info (No color)
     * 4 - > Debug (Gray)
     *
    */
    bool ok = false;
    static int logLevel = qEnvironmentVariable("KALAMI_LOG_LEVEL", "0").toInt(&ok) ;
    if (!ok)
        logLevel = 4;

    switch (type) {
    case QtDebugMsg:
        if (logLevel < 4)
            return;
        fprintf(stderr, GRAY_HI "%s: " GRAY_LO "%s\n" NORMAL, context.category, msg.toUtf8().data());
        break;
    case QtInfoMsg:
        if (logLevel < 3)
            return;
        fprintf(stderr, NORMAL "%s: " NORMAL "%s\n" NORMAL, context.category, msg.toUtf8().data());
        break;
    case QtWarningMsg:
        if (logLevel < 2)
            return;
        fprintf(stderr, YELLOW_HI "%s: " YELLOW_LO "%s\n" NORMAL, context.category, msg.toUtf8().data());
        break;
    case QtCriticalMsg:
        if (logLevel < 1)
            return;
        fprintf(stderr, RED_HI "%s: " RED_LO "%s\n" NORMAL, context.category, msg.toUtf8().data());
        break;
    case QtFatalMsg:
        fprintf(stderr, RED_HI "%s: " RED_LO "%s\n" NORMAL, context.category, msg.toUtf8().data());
        break;
    }
}


int main(int argc, char *argv[])
{
    qInstallMessageHandler(kalamiMessageOutput);

    QCoreApplication app(argc, argv);
    QCommandLineParser parser;

    parser.setApplicationDescription("Hardware abstraction layer for Nepos embedded devices");
    parser.addHelpOption();

    QCommandLineOption serverOption(QStringList() <<
                                    "s" << "server",
                                    "Websocket URI to connect to",
                                    QStringLiteral("server"), QStringLiteral("ws://localhost:3010/ws/kalami"));
    parser.addOption(serverOption);
    parser.process(app);

    Daemon d(QUrl(parser.value(serverOption)), &app);

    if (!d.init())
        return EXIT_FAILURE;

    return app.exec();
}
