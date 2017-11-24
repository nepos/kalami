#include <QDebug>
#include <alsa/asoundlib.h>
#include <math.h>
#include "alsamixer.h"

Q_LOGGING_CATEGORY(ALSAMixerLog, "ALSAMixer")

struct ALSAMixerPrivate {
    ALSAMixerPrivate() {};

    snd_mixer_t *handle;
    long masterMin, masterMax;
    float masterCurrent, masterScale;
};

static snd_mixer_elem_t *findMixerElement(snd_mixer_t *handle, const char *name, int index)
{
    snd_mixer_selem_id_t *sid;

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, index);
    snd_mixer_selem_id_set_name(sid, name);

    return snd_mixer_find_selem(handle, sid);
}

ALSAMixer::ALSAMixer(const QString &deviceName, QObject *parent) :
    QObject(parent), d_ptr(new ALSAMixerPrivate)
{
    int ret;
    Q_D(ALSAMixer);

    memset(d, 0, sizeof(ALSAMixerPrivate));
    d->masterCurrent = -INFINITY;
    d->masterScale = 1.0f;

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

    // Set ALSA mixer controls
    setEnumByName("RX1 MIX1 INP1", "RX1");
    setEnumByName("RX2 MIX1 INP1", "RX2");
    setEnumByName("RDAC2 MUX", "RX2");
    setEnumByName("HPHL", "Switch");
    setEnumByName("HPHR", "Switch");

    // Read volume ranges for master volume control, so we can scale
    snd_mixer_elem_t *me = findMixerElement(d->handle, "RX1 Digital", 0);
    if (me)
        snd_mixer_selem_get_playback_volume_range(me, &d->masterMin, &d->masterMax);
    else
        qWarning(ALSAMixerLog) << "Unable to find playback mixer element";

    getMasterVolume();
}

ALSAMixer::~ALSAMixer()
{
    Q_D(ALSAMixer);

    if (d->handle)
        snd_mixer_close(d->handle);
}

void ALSAMixer::setMasterScale(float scale)
{
    Q_D(ALSAMixer);

    d->masterScale = scale;
}

bool ALSAMixer::setPlaybackVolumeByName(const char *name, int val, int index)
{
    Q_D(ALSAMixer);

    snd_mixer_elem_t *me = findMixerElement(d->handle, name, index);
    if (!me) {
        qWarning(ALSAMixerLog) << "Unable to find playback mixer element named" << name;
        return false;
    }

    return snd_mixer_selem_set_playback_volume(me, SND_MIXER_SCHN_FRONT_LEFT, val) >= 0;
}

void ALSAMixer::setEnumByName(const char *name, const char *value, int index)
{
    Q_D(ALSAMixer);
    int ret;

    snd_mixer_elem_t *me = findMixerElement(d->handle, name, index);
    if (!me) {
        qWarning(ALSAMixerLog) << "Unable to find playback mixer element named" << name;
        return;
    }

    for (int i = 0; i < snd_mixer_selem_get_enum_items(me); i++) {
        char buf[256];

        ret = snd_mixer_selem_get_enum_item_name(me, i, sizeof(buf) - 1, buf);
        if (ret < 0)
            continue;

        if (strcmp(buf, value) == 0) {
            ret = snd_mixer_selem_set_enum_item(me, SND_MIXER_SCHN_MONO, i);
            if (ret == 0)
                return;
        }
    }

    qWarning(ALSAMixerLog) << "Unable to find enum value" << value << "in mixer element!";
}

float ALSAMixer::getMasterVolume()
{
    Q_D(ALSAMixer);

    snd_mixer_elem_t *me = findMixerElement(d->handle, "RX1 Digital", 0);
    if (!me) {
        qWarning(ALSAMixerLog) << "Unable to find playback mixer element";
        d->masterCurrent = 0.0f;
    } else {
        int ret;
        long current;

        ret = snd_mixer_selem_get_playback_volume(me, SND_MIXER_SCHN_FRONT_LEFT, &current);
        if (ret < 0)
            d->masterCurrent = 0.0f;
        else
            d->masterCurrent = (float) (d->masterCurrent - d->masterMin) / (float) (d->masterMax - d->masterMin);
    }

    return d->masterCurrent / d->masterScale;
}

bool ALSAMixer::setMasterVolume(float volume)
{
    Q_D(ALSAMixer);

    float val = d->masterMin + (volume * (float) (d->masterMax - d->masterMin));

    val *= d->masterScale;

    return setPlaybackVolumeByName("RX1 Digital", val) &&
            setPlaybackVolumeByName("RX2 Digital", val);
}
