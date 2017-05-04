#ifndef AUDIO_SETTINGS_H
#define AUDIO_SETTINGS_H

#include "../lib/macros.h"
#include "../lib/bb_lib.h"

#include <QSettings>

/*
 * All settings needed to send to the device
 *   to retrieve all variations of audio
 */
class AudioSettings : public QObject {
    Q_OBJECT

public:
    AudioSettings();
    AudioSettings(const AudioSettings &other);
    ~AudioSettings() {}

    AudioSettings& operator=(const AudioSettings &other);
    bool operator==(const AudioSettings &other) const;
    bool operator!=(const AudioSettings &other) const;

    void LoadDefaults();
    bool Load(QSettings &s);
    bool Save(QSettings &s) const;

    int AudioMode() const { return demod_mode; }
    Frequency CenterFreq() const { return center_freq; }
    Frequency IFBandwidth() const { return if_bandwidth; }
    Frequency LowPassFreq() const { return low_pass_freq; }
    Frequency HighPassFreq() const { return high_pass_freq; }
    double FMDeemphasis() const { return fm_deemphasis; }

private:
    int demod_mode;
    Frequency center_freq;
    Frequency if_bandwidth;
    Frequency low_pass_freq;
    Frequency high_pass_freq;
    double fm_deemphasis;

public slots:
    void setMode(int);
    void setCenterFrequency(Frequency);
    void setIFBandwidth(Frequency);
    void setLowPassFreq(Frequency);
    void setHighPassFreq(Frequency);
    void setFMDeemphasis(double);

signals:
    void updated(const AudioSettings*);
};

#endif // AUDIO_SETTINGS_H
