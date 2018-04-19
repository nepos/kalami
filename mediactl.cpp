#include <QDebug>
#include <QProcess>
#include "mediactl.h"

Q_LOGGING_CATEGORY(MediaCtlLog, "MediaCtl")

MediaCtl::MediaCtl(int index, QObject *parent) : QObject(parent), sensorNames()
{
    mediaDevice = "/dev/media" + QString::number(index);
    sensorNames << "ov5640 1-003c";
    sensorNames << "ov5640 1-004c";
}

bool MediaCtl::initialize()
{
    return reset() && makeLinks();
}

bool MediaCtl::invokeBinary(const QStringList &extraArgs)
{
    QStringList args;

    args << "-d" << mediaDevice;
    args << extraArgs;

    qInfo(MediaCtlLog) << "Calling: /usr/bin/media-ctl" << args.join(" ");

    QProcess proc(this);
    proc.setProgram("/usr/bin/media-ctl");
    proc.setArguments(args);
    proc.start();
    proc.waitForFinished();

    auto ret =
            proc.exitStatus() == QProcess::NormalExit &&
            proc.exitCode() == 0;
    if (!ret) {
        qInfo(MediaCtlLog) << "Error while calling media-ctl:";
        qInfo(MediaCtlLog) << proc.readAll();
    }

    return ret;
}

bool MediaCtl::reset()
{
    return invokeBinary(QStringList("-r"));
}

bool MediaCtl::makeLinks()
{
    for (int i = 0; i < 2; ++i) {
        QStringList links;
        links << QString::asprintf("\"msm_csiphy%d\":1->\"msm_csid%d\":0[1]", i, i);
        links << QString::asprintf("\"msm_csid%d\":1->\"msm_ispif%d\":0[1]", i, i);
        links << QString::asprintf("\"msm_ispif%d\":1->\"msm_vfe0_rdi%d\":0[1]", i, i);

        QStringList args;
        args << "-l";
        args << links.join(",");

        if (!invokeBinary(args))
            return false;
    }

    return true;
}

bool MediaCtl::setConfig(int index, enum Config config)
{
    QString configString;

    switch (config) {
    case UYVY8_2X8_1920x1080:
        configString = "UYVY8_2X8/1920x1080";
        break;
    case UYVY8_2X8_2592x1944:
        configString = "UYVY8_2X8/2592x1944";
        break;
    default:
        qWarning(MediaCtlLog) << "Unsupported config" << config;
        return false;
    }

    QStringList pads;
    pads << sensorNames.at(index);
    pads << "msm_csiphy" + QString::number(index);
    pads << "msm_csid" + QString::number(index);
    pads << "msm_ispif" + QString::number(index);
    pads << "msm_vfe0_rdi" + QString::number(index);

    QStringList configs;

    foreach (QString pad, pads) {
        configs << QString::asprintf("\"%s\":0[fmt:%s field:none]",
                                     pad.toUtf8().constData(),
                                     configString.toUtf8().constData());
    }

    QStringList args;
    args << "-V";
    args << configs.join(",");

    return invokeBinary(args);
}
