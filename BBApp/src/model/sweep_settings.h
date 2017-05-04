#ifndef SWEEP_SETTINGS_H
#define SWEEP_SETTINGS_H

#include "../lib/bb_lib.h"

#include <QObject>

class QSettings;

/*
 * SA44 Settings
 */
class SweepSettings : public QObject {
    Q_OBJECT

public:
    SweepSettings();
    SweepSettings(const SweepSettings &other);
    ~SweepSettings() {}

    SweepSettings& operator=(const SweepSettings &other);
    bool operator==(const SweepSettings &other) const;
    bool operator!=(const SweepSettings &other) const;

    void LoadDefaults();
    bool Load(QSettings &s);
    bool Save(QSettings &s) const;

    OperationalMode Mode() const { return mode; }
    bool IsRealTime() const { return mode == MODE_REAL_TIME; }

    // Freq getters
    Frequency Start() const { return start; }
    Frequency Stop() const { return stop; }
    Frequency Center() const { return center; }
    Frequency Span() const { return span; }
    Frequency Step() const { return step; }
    Frequency RBW() const { return rbw; }
    Frequency VBW() const { return vbw; }

    bool AutoRBW() const { return auto_rbw; }
    bool AutoVBW() const { return auto_vbw; }
    bool NativeRBW() const { return native_rbw; }

    Amplitude RefLevel() const { return refLevel; }
    double Div() const { return div; }
    int Atten() const { return attenuation; }
    int Gain() const { return gain; }
    int Preamp() const { return preamp; }

    Time SweepTime() const { return sweepTime; }
    int ProcessingUnits() const { return processingUnits; }
    int Detector() const { return detector; }
    bool Rejection() const { return rejection; }

    // Call when you want to update the control panel
    void EmitUpdated() { emit updated(this); }

    // Return string for displaying current attenuation value
    void GetAttenString(QString &str) const {
        if(attenuation == 0) {
            str = "Atten Auto";
        } else {
            str.sprintf("Atten %d", (attenuation-1) * 10);
        }
    }

    // Return 2.02 for native, or other for non-native
    double GetWindowBandwidth() const {
        if(NativeRBW()) {
            return 2.02;
        } else {
            double b = device_traits::get_flattop_window_bandwidth(rbw);
            return b;
        }
    }

    // Returns true if settings fine for CP/OCBW
    bool IsAveragePower() const;

    Amplitude refLevel;

    int tgSweepSize;
    bool tgHighRangeSweep;
    bool tgPassiveDevice;

protected:

private:
    void AutoBandwidthAdjust(bool force);
    void UpdateProgram();

    OperationalMode mode;

    Frequency start;
    Frequency stop;
    Frequency center;
    Frequency span;
    Frequency step;
    Frequency rbw;
    Frequency vbw;

    bool auto_rbw;
    bool auto_vbw;
    bool native_rbw;

    double div; // dB
    int attenuation;
    int gain; // From 0 -> 4, where 0 equals auto, 1-4 = levels
    int preamp; // auto = 0, off = 1, on = 2

    Time sweepTime;
    int processingUnits;
    int detector;
    // Set in Settings File Menu
    bool rejection;

signals:
    // Emit when settings successfully changed
    void updated(const SweepSettings*);

public slots:
    // Official setters manipulated from control panel
    // Each slot setter must emit updated() only once
    //   at the end of the function regardless of whether
    //   values were updated or not.
    void setMode(OperationalMode _mode);

    void setStart(Frequency);
    void setStop(Frequency);
    void setCenter(Frequency);
    void increaseCenter(bool);
    void setSpan(Frequency);
    void increaseSpan(bool);
    void setStep(Frequency);
    void setFullSpan();

    void setRBW(Frequency);
    void setVBW(Frequency);
    void rbwIncrease(bool);
    void vbwIncrease(bool);

    void setAutoRbw(bool);
    void setAutoVbw(bool);
    void setNativeRBW(bool);

    void setRefLevel(Amplitude);
    void shiftRefLevel(bool);
    void setDiv(double);
    void setAttenuation(int);
    void setGain(int);
    void setPreAmp(int);

    void setSweepTime(Time);
    void setProcUnits(int);
    void setDetector(int);
    void setRejection(bool);

    void setTgSweepSize(double);
    void setTgHighRange(bool);
    void setTgPassiveDevice(int);
};

#endif // SWEEP_SETTINGS_H
