#ifndef MARKER_H
#define MARKER_H

#include "../lib/macros.h"
#include "../lib/frequency.h"
#include "../lib/amplitude.h"
#include "../lib/time_type.h"

class Trace;
class SweepSettings;

class Marker {
public:
    Marker();
    ~Marker();

    void Reset();

    bool Active() const { return active; }
    bool DeltaActive() const { return deltaOn; }
    bool Updating() const { return update; }
    int OnTrace() const { return onTrace; }
    bool InView() const { return inView; }
    bool DeltaInView() const { return deltaInView; }
    Frequency Freq() const { return freq; }
    Amplitude Amp() const { return amp; }
    Amplitude DeltaAmp() const { return deltaAmp; }

    int Index() const { return index; }
    int DeltaIndex() const { return deltaIndex; }
    double xRatio() const { return xr; }
    double yRatio() const { return yr; }
    double delxRatio() const { return delta_xr; }
    double delyRatio() const { return delta_yr; }
    QString Text() const { return text; }
    QString DeltaText() const { return deltaText; }

    void SetOnTrace(int t) { onTrace = t; }
    void SetActive(bool a);
    void SetUpdate(bool u) { update = u; }
    // Add adjust to frequency
    void AdjustFrequency(Frequency adjust, bool right);
    void AdjustMarker(bool increase);

    void EnableDelta();

    // Place the marker at a given frequency, and
    //   activate it if it is not already
    //bool Place(double percent);
    //bool Place(int newIndex);
    //bool Place(Time t);
    bool Place(Frequency f);
    bool Place(Frequency f, double percent);
    //bool Place(int index); // For phase noise

    // Use the frequency to determine the index, amplitude
    //  and delta values, do nothing if not active
    void UpdateMarker(const Trace *trace, const SweepSettings *s);
    void UpdateMarkerForPhaseNoise(const Trace *trace, const SweepSettings *s);
    //void UpdateMarker(const Trace* trace, const DemodSettings *ds);

private:
    bool active;
    bool update;
    bool deltaOn;
    int onTrace; // Which trace the marker is on

    bool inView; // Is the marker in view? updated each sweep
    bool deltaInView;

    int index, deltaIndex;
    Frequency freq, deltaFreq;
    Amplitude amp, deltaAmp;
    Time time;

    // View variables, simplify drawing
    double xr, yr; // x/y ratios 0.0 -> 1.0
    double delta_xr, delta_yr;
    QString text, deltaText;
};

#endif // MARKER_H
