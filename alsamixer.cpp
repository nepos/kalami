#include <QDebug>
#include <alsa/asoundlib.h>
#include <math.h>
#include "alsamixer.h"

Q_LOGGING_CATEGORY(ALSAMixerLog, "ALSAMixer")

struct ALSAMixerPrivate {
    ALSAMixerPrivate() {};

    snd_mixer_t *handle;
    snd_mixer_elem_t *masterElement;
    long masterMin, masterMax;
    float masterCurrent;
};

ALSAMixer::ALSAMixer(const QString &deviceName, QObject *parent) :
    QObject(parent), d_ptr(new ALSAMixerPrivate)
{
    int ret;
    Q_D(ALSAMixer);

    memset(d, 0, sizeof(ALSAMixerPrivate));
    d->masterCurrent = -INFINITY;

    ret = snd_mixer_open(&d->handle, 0);
    if (ret < 0) {
        qInfo(ALSAMixerLog) << "Unable to open ALSA mixer interface:" << strerror(-errno);
        return;
    }

    ret = snd_mixer_attach(d->handle, deviceName.toLocal8Bit().constData());
    if (ret < 0) {
        qInfo(ALSAMixerLog) << "Unable to open ALSA mixer device handle:" << strerror(-errno);
        return;
    }

    snd_mixer_selem_register(d->handle, NULL, NULL);
    snd_mixer_load(d->handle);

    qInfo(ALSAMixerLog) << "ALSA mixer interface opened for" << deviceName;

    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "Master");
    d->masterElement = snd_mixer_find_selem(d->handle, sid);
    if (!d->masterElement) {
        qInfo(ALSAMixerLog) << "Unable to find ALSA mixer element for master volume:" << strerror(-errno);
        return;
    }

    snd_mixer_selem_get_playback_volume_range(d->masterElement, &d->masterMin, &d->masterMax);
    getMasterVolume();
}

ALSAMixer::~ALSAMixer()
{
    Q_D(ALSAMixer);

    if (d->masterElement)
        snd_mixer_elem_free(d->masterElement);

    if (d->handle)
        snd_mixer_close(d->handle);
}

float ALSAMixer::getMasterVolume()
{
    Q_D(ALSAMixer);

    long current;
    int ret;

    if (!d->masterElement)
        return 0.0;

    ret = snd_mixer_selem_get_playback_volume(d->masterElement, SND_MIXER_SCHN_FRONT_LEFT, &current);
    if (ret < 0)
        return 0.0;

    d->masterCurrent = (float) (d->masterCurrent - d->masterMin) / (float) (d->masterMax - d->masterMin);

    return d->masterCurrent;
}

void ALSAMixer::setMasterVolume(float volume)
{
    Q_D(ALSAMixer);

    if (d->masterElement) {
        float val = d->masterMin + (volume * (float) (d->masterMax - d->masterMin));

        if (val != d->masterCurrent)
            snd_mixer_selem_set_playback_volume_all(d->masterElement, val);
    }
}
