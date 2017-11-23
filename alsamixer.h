#pragma once

#include <QtCore/QLoggingCategory>
#include <QObject>

Q_DECLARE_LOGGING_CATEGORY(ALSAMixerLog)

struct ALSAMixerPrivate;

class ALSAMixer : public QObject
{
    Q_OBJECT
public:
    explicit ALSAMixer(const QString &deviceName = "default", QObject *parent = 0);
    ~ALSAMixer();

    float getMasterVolume();

public slots:
    bool setMasterVolume(float volume);
    void setMasterScale(float scale);

private:
    ALSAMixerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(ALSAMixer);
    bool setPlaybackVolumeByName(const char *name, int val, int index = 0);
    void setEnumByName(const char *name, const char *value, int index = 0);
};
