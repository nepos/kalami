#include <QDebug>
#include <QProcess>
#include "mediactl.h"

Q_LOGGING_CATEGORY(MediaCtlLog, "MediaCtl")

MediaCtl::MediaCtl(int index, QObject *parent) : QObject(parent), sensorNames()
{
    mediaDevice = "/dev/media" + QString::number(index);
    sensorNames << "ov5640 1-003c";
    sensorNames << "ov5640 1-003x"; // !!
}

bool MediaCtl::initialize()
{
    return reset() && makeLinks();
}

bool MediaCtl::invokeBinary(const QStringList extraArgs)
{
    QStringList args;

    args << "-d" << mediaDevice;
    args << extraArgs;

    QProcess proc(this);
    proc.start("/usr/bin/media-ctl", args);
    proc.waitForFinished();

    return proc.exitStatus() == QProcess::NormalExit &&
            proc.exitCode() == 0;
}

bool MediaCtl::reset()
{
    return invokeBinary(QStringList("-r"));
}

bool MediaCtl::makeLinks()
{
    for (int i = 0; i < 2; ++i) {
        QStringList args;
        QStringList links;

        links << QString::asprintf("\"msm_csiphy%d\":1->\"msm_csid%d\":0[1]", i, i);
        links << QString::asprintf("\"msm_csid%d\":1->\"msm_ispif%d\":0[1]", i, i);
        links << QString::asprintf("\"msm_ispif%d\":1->\"msm_vfe0_rdi%d\":0[1]", i, i);

        args << "-l";
        args << "'" + links.join(",") + "'";

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
        configString = "UYVY8_2X8_1920x1080";
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
    args << "'" + configs.join(",") + "'";

    return invokeBinary(args);
}
