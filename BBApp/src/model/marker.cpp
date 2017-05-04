#include "marker.h"
#include "trace.h"

Marker::Marker()
{
    Reset();
}

Marker::~Marker()
{

}

// Called when the delta button is pressed
void Marker::EnableDelta()
{
    if(!inView) {
        return;
    }

    if(deltaOn) {
        deltaOn = false;
        return;
    }

    deltaOn = true;
    deltaIndex = index;
    deltaFreq = freq;
    deltaAmp = amp;
    delta_xr = xr;
    delta_yr = yr;
}

void Marker::Reset()
{
    active = false;
    update = true;
    deltaOn = false;
    onTrace = 0;

    inView = deltaInView = false;

    index = deltaIndex = 0;
    freq = deltaFreq = 0.0;

    xr = yr = 0.0;
    delta_xr = delta_yr = 0.0;
    text = "";
}

void Marker::SetActive(bool a)
{
    active = a;
}

void Marker::AdjustFrequency(Frequency adjust, bool right)
{
    if(active) {
        freq += adjust;
    }

    if(right) {
        index++;
    } else {
        index--;
    }
}

void Marker::AdjustMarker(bool increase)
{
    if(increase) {
        index--;
        if(index < 0) index = 0;
    } else {
        index++;
    }
}

//bool Marker::Place(double percent)
//{
//    xr = percent;

//    if(!active) {
//        active = true;
//    }

//    return true;
//}

//bool Marker::Place(int newIndex)
//{
//    index = newIndex;

//    if(!active) {
//        active = true;
//    }

//    return true;
//}

//// Return true if the marker was activated
//bool Marker::Place(Time t)
//{
//    time = t;

//    if(!active) {
//        active = true;
//        return true;
//    }

//    return false;
//}

bool Marker::Place(Frequency f)
{
    freq = f;

    if(!active) {
        active = true;
        return true;
    }

    return true;
}

bool Marker::Place(Frequency f, double percent)
{
    freq = f;
    xr = percent;

    if(!active) {
        active = true;
        return true;
    }

    return true;
}

/*
 * 1) Use freq to clamp to an index
 * 2) Use index to find absolute freq and amplitude
 */
void Marker::UpdateMarker(const Trace* trace,
                          const SweepSettings *s)
{
    if(!active || !update) return;
    if(!trace->Active()) {
        inView = false;
        return;
    }

    // Regular marker determined by frequency
    Frequency num = freq - trace->StartFreq();// + 0.5;
    //index = xr * trace->Length();
    index = (int)((num / trace->BinSize()) + 0.5);
    freq = trace->StartFreq() + index * trace->BinSize();
    bool log_scale = s->RefLevel().IsLogScale();

    if(index < 0 || index >= trace->Length()) {
        inView = false;
    } else {
        inView = true;

        double level = trace->Max()[index];
        AmpUnits units = s->RefLevel().Units();

        xr = (double)index / (trace->Length()-1);
        if(log_scale) {
            yr = level - (s->RefLevel().ConvertToUnits(DBM) - (10.0 * s->Div()));
            yr /= (10.0 * s->Div());
            amp = Amplitude(level, DBM).ConvertToUnits(units);
        } else {
            yr = level / s->RefLevel().Val();
            amp = Amplitude(level, MV);
        }
        bb_lib::clamp(yr, 0.0, 1.0);

        text = (freq).GetFreqString(6, true) +
                ", " + amp.GetString();
    }

    // Delta marker
    if(deltaOn) {
        QString unit_string = log_scale ? "dB" : "mV";
        Frequency num = deltaFreq - trace->StartFreq();
        deltaIndex = (int)((num / trace->BinSize()) + 0.5);
        deltaFreq = trace->StartFreq() + deltaIndex * trace->BinSize();

        if(deltaIndex < 0 || deltaIndex >= trace->Length()) {
            deltaInView = false;
        } else {
            deltaInView = true;

            delta_xr = (double)deltaIndex / (trace->Length()-1);
            if(log_scale) {
                delta_yr = deltaAmp.Val() - (s->RefLevel() - (10.0 * s->Div()));
                delta_yr /= (10.0 * s->Div());
            } else {
                delta_yr = deltaAmp.Val() / s->RefLevel().Val();
            }
            bb_lib::clamp(delta_yr, 0.0, 1.0);
        }
        // Update delta text even if not in view
        Amplitude dif(amp.Val() - deltaAmp.Val(), s->RefLevel().Units());
        deltaText = (freq - deltaFreq).GetFreqString(6, true) +
                ", " + dif.GetValueString() + unit_string;
    }
}

void Marker::UpdateMarkerForPhaseNoise(const Trace *trace, const SweepSettings *s)
{
    if(!active || !update) return;
    if(!trace->Active()) {
        inView = false;
        return;
    }

    index = xr * trace->Length();
    if(index >= 0 && index < trace->Length()) {
        inView = true;
        xr = double(index) / (trace->Length() - 1);
        amp = Amplitude(trace->Max()[index], DBM);
        yr = amp.Val() - (s->RefLevel().ConvertToUnits(DBM) - (10.0 * s->Div()));
        yr /= (10.0 * s->Div());
        bb_lib::clamp(yr, 0.0, 1.0);

        text = (freq).GetFreqString(6, true) +
                ", " + amp.GetString();
    } else {
        inView = false;
    }

    if(deltaOn) {
        if(deltaIndex >= 0 && deltaIndex < trace->Length()) {
            deltaInView = true;
            delta_xr = (double)deltaIndex / (trace->Length()-1);
            delta_yr = deltaAmp.Val() - (s->RefLevel() - (10.0 * s->Div()));
            delta_yr /= (10.0 * s->Div());
            bb_lib::clamp(delta_yr, 0.0, 1.0);
        } else {
            deltaInView = false;
        }
        // Update delta text even if not in view
        Amplitude dif(amp.Val() - deltaAmp.Val(), s->RefLevel().Units());
        deltaText = (freq - deltaFreq).GetFreqString(6, true) +
                ", " + dif.GetValueString() + "dBc/Hz";
    }
}

//void Marker::UpdateMarker(const Trace *trace, const DemodSettings *ds)
//{
//    if(!active || !update) {
//        return;
//    }

//    if(!trace->Active()) {
//        inView = false;
//        return;
//    }

//    index = time / (trace->Length() * trace->BinSize());
//    if(index < 0 || index >= trace->Length()) {
//        inView = false;
//        return;
//    }

//    time = index * trace->BinSize();
//    xr = (double)index / trace->Length();

//    // y-axis and text relies on the type of modulation
//    if(ds->Mode() == DemodSettings::AM_DEMOD) {
//        Amplitude aamp;
//        aamp = trace->Max()[index];
//        yr = aamp - (ds->AmpRef() - (10.0 * ds->Div()));
//        yr /= (10.0 * ds->Div());
//        text = time.GetString() + ", " + aamp.GetString();
//    } else { // FM_DEMOD
//        Frequency famp = trace->Max()[index];
//        yr = (famp + ds->FreqRef()) / (ds->FreqRef() * 2.0);
//        text = time.GetString() + ", " + famp.GetFreqString();
//    }
//}

