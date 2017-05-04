#ifndef DEMOD_SETTINGS_H
#define DEMOD_SETTINGS_H

#include "lib/bb_lib.h"

#include <QSettings>

enum TriggerType {
    TriggerTypeNone = 0,
    TriggerTypeVideo = 1,
    TriggerTypeExternal = 2
};

enum TriggerEdge {
    TriggerEdgeRising = 0,
    TriggerEdgeFalling = 1
};

class DemodSettings : public QObject {
    Q_OBJECT

public:
    DemodSettings();
    DemodSettings(const DemodSettings &other);
    ~DemodSettings() {}

    DemodSettings& operator=(const DemodSettings &other);
    bool operator==(const DemodSettings &other) const;
    bool operator!=(const DemodSettings &other) const;

    void LoadDefaults();
    bool Load(QSettings &s);
    bool Save(QSettings &s) const;

    Amplitude InputPower() const { return inputPower; }
    Frequency CenterFreq() const { return centerFreq; }
    int Gain() const { return gain; }
    int Atten() const { return atten; }
    int Preamp() const { return preamp; }
    int DecimationFactor() const { return decimationFactor; }
    Frequency Bandwidth() const { return bandwidth; }
    bool AutoBandwidth() const { return autoBandwidth; }
    Time SweepTime() const { return sweepTime; }

    TriggerType TrigType() const { return trigType; }
    TriggerEdge TrigEdge() const { return trigEdge; }
    Amplitude TrigAmplitude() const { return trigAmplitude; }
    double TrigPosition() const { return trigPosition; }

    bool MAEnabled() const { return maEnabled; }
    Frequency MALowPass() const { return maLowPass; }

private:
    // Call before updating, configures an appropriate sweep time value
    void ClampSweepTime();
    void UpdateAutoBandwidths();
    void SetMRConfiguration();

    Amplitude inputPower;
    Frequency centerFreq;
    int gain; // Index, 0 == auto
    int atten; // Index, 0 == auto
    int preamp; // Index, 0 == auto
    int decimationFactor;
    Frequency bandwidth;
    bool autoBandwidth;
    Time sweepTime;

    TriggerType trigType;
    TriggerEdge trigEdge;
    Amplitude trigAmplitude;
    double trigPosition; // 0 - 90 %

    bool maEnabled; // Mod-Analysis enabled
    Frequency maLowPass; // Mod-analysis low pass filter on audio

public slots:
    void setInputPower(Amplitude);
    void setCenterFreq(Frequency);
    void setGain(int);
    void setAtten(int);
    void setDecimation(int);
    void setBandwidth(Frequency);
    void setAutoBandwidth(bool);
    void setSweepTime(Time);

    void setTrigType(int);
    void setTrigEdge(int);
    void setTrigAmplitude(Amplitude);
    void setTrigPosition(double);

    void setMAEnabled(bool);
    void setMALowPass(Frequency);

signals:
    void updated(const DemodSettings*);
};

// Descriptor for the device IQ stream
struct IQDescriptor {
    IQDescriptor() {
        sampleRate = 0.0;
        decimation = 0;
        timeDelta = 0.0;
        returnLen = 0;
        bandwidth = 0.0;
    }

    double sampleRate;
    int decimation;
    double timeDelta; // Delta 'time' per sample in seconds?
    int returnLen;
    double bandwidth;
};

// Represents a single IQ capture from the device
struct IQCapture {
    IQCapture() {
        simdZero_32s(triggers, 70);
    }
    ~IQCapture() {}

    std::vector<complex_f> capture;
    int triggers[70];
};

struct ModAnalysisReport {
    ModAnalysisReport() {
        rfCenter = 0.0;
        fmRMS = amRMS = 0.0;
        fmPeakPlus = fmPeakMinus = 0.0;
        amPeakPlus = amPeakMinus = 0.0;
        fmAudioFreq = amAudioFreq = 0.0;
        fmSINAD = fmTHD = 0.0;
        amSINAD = amTHD = 0.0;
    }

    double rfCenter; // Avg of fm frequencies
    double fmRMS, amRMS;
    double fmPeakPlus, fmPeakMinus;
    double amPeakPlus, amPeakMinus;
    double fmAudioFreq, amAudioFreq;
    double fmSINAD, fmTHD;
    double amSINAD, amTHD;
};

// Represents a full IQ sweep and all data needed to update all views
typedef struct IQSweep {
    IQSweep() : sweepLen(0), dataLen(0), preTrigger(0), triggered(false) {}

    DemodSettings settings;
    IQDescriptor descriptor;
    std::vector<complex_f> iq;     // Stores an even number of IQ captures
    int sweepLen;                  // The requested length of the sweep
    int dataLen;                   // Number of actual samples in buffer
                                   // May be longer than sweepLen
    int preTrigger;
    std::vector<float> amWaveform; // (i*i + q*q)
    std::vector<float> fmWaveform; // Frequency over time
    std::vector<float> pmWaveform; // Phase over time
    ModAnalysisReport stats;
    bool triggered;

    // Convert IQ to AM/FM/PM waveforms
    void Demod();
    // From AM/FM/PM waveforms, get receiver stats
    void CalculateReceiverStats();
} IQSweep;

// waveform: filtered waveform
double CalculateSINAD(const std::vector<double> &waveform, double sampleRate, double centerFreq);
double CalculateTHD(const std::vector<double> &waveform, double sampleRate, double centerFreq);

#endif // DEMOD_SETTINGS_H
