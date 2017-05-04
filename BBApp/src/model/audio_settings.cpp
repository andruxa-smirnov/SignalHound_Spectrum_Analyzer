#include "audio_settings.h"
#include "../lib/bb_api.h"

AudioSettings::AudioSettings()
{
    LoadDefaults();
}

AudioSettings::AudioSettings(const AudioSettings &other)
{
    *this = other;
}

AudioSettings& AudioSettings::operator=(const AudioSettings &other)
{
    demod_mode = other.demod_mode;
    center_freq = other.center_freq;
    if_bandwidth = other.if_bandwidth;
    low_pass_freq = other.low_pass_freq;
    high_pass_freq = other.high_pass_freq;
    fm_deemphasis = other.fm_deemphasis;

    return *this;
}

bool AudioSettings::operator==(const AudioSettings &other) const
{
    if(demod_mode != other.demod_mode) return false;
    if(center_freq != other.center_freq) return false;
    if(if_bandwidth != other.if_bandwidth) return false;
    if(low_pass_freq != other.low_pass_freq) return false;
    if(high_pass_freq != other.high_pass_freq) return false;
    if(fm_deemphasis != other.fm_deemphasis) return false;

    return true;
}

bool AudioSettings::operator!=(const AudioSettings &other) const
{
    return !(*this == other);
}

void AudioSettings::LoadDefaults()
{
    demod_mode = BB_DEMOD_FM;
    center_freq = 97.1e6;
    if_bandwidth = 120.0e3;
    low_pass_freq = 8.0e3;
    high_pass_freq = 20.0;
    fm_deemphasis = 75.0; // us
}

bool AudioSettings::Load(QSettings &s)
{
    demod_mode = s.value("Audio/DemodMode", AudioMode()).toInt();
    center_freq = s.value("Audio/CenterFreq", center_freq.Val()).toDouble();
    if_bandwidth = s.value("Audio/IFBandwidth", if_bandwidth.Val()).toDouble();
    low_pass_freq = s.value("Audio/LowPassFreq", low_pass_freq.Val()).toDouble();
    high_pass_freq = s.value("Audio/HighPassFreq", high_pass_freq.Val()).toDouble();
    fm_deemphasis = s.value("Audio/FMDeemphasis", fm_deemphasis).toDouble();

    emit updated(this);
    return true;
}

bool AudioSettings::Save(QSettings &s) const
{
    s.setValue("Audio/DemodMode", demod_mode);
    s.setValue("Audio/CenterFreq", center_freq.Val());
    s.setValue("Audio/IFBandwidth", if_bandwidth.Val());
    s.setValue("Audio/LowPassFreq", low_pass_freq.Val());
    s.setValue("Audio/HighPassFreq", high_pass_freq.Val());
    s.setValue("Audio/FMDeemphasis", fm_deemphasis);

    return true;
}

// Modes between 0x0 and 0x4
//  values match audio modes in api header
void AudioSettings::setMode(int new_mode)
{
    bb_lib::clamp(new_mode, 0x0, 0x4);

    if(demod_mode != new_mode) {
        demod_mode = new_mode;
        emit updated(this);
    }
}

void AudioSettings::setCenterFrequency(Frequency new_center_freq)
{
    bb_lib::clamp(new_center_freq,
                  Frequency(100.0e3),
                  Frequency(6.0e9));

    center_freq = new_center_freq;
    emit updated(this);
}

void AudioSettings::setIFBandwidth(Frequency new_if_bandwidth)
{
    bb_lib::clamp(new_if_bandwidth,
                  Frequency(500.0),
                  Frequency(200.0e3));

    if_bandwidth = new_if_bandwidth;
    emit updated(this);
}

void AudioSettings::setLowPassFreq(Frequency new_low_pass_freq)
{
    bb_lib::clamp(new_low_pass_freq,
                  Frequency(3.0e3),
                  Frequency(12.0e3));

    low_pass_freq = new_low_pass_freq;
    emit updated(this);
}

void AudioSettings::setHighPassFreq(Frequency new_high_pass_freq)
{
    bb_lib::clamp(new_high_pass_freq,
                  Frequency(20.0),
                  Frequency(100.0));

    high_pass_freq = new_high_pass_freq;
    emit updated(this);
}

void AudioSettings::setFMDeemphasis(double new_fm_deemphasis)
{
    bb_lib::clamp(new_fm_deemphasis,
                  10.0, 150.0);

    fm_deemphasis = new_fm_deemphasis;
    emit updated(this);
}
