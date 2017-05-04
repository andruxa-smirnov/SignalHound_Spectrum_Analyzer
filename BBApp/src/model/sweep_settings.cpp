#include "sweep_settings.h"

#include <QSettings>

#include "lib/bb_api.h"
#include "lib/device_traits.h"

SweepSettings::SweepSettings()
{
    LoadDefaults();    
}

SweepSettings::SweepSettings(const SweepSettings &other)
{
    *this = other;
}

SweepSettings& SweepSettings::operator=(const SweepSettings &other)
{
    mode = other.mode;

    start = other.start;
    stop = other.stop;
    center = other.center;
    span = other.span;
    step = other.step;
    rbw = other.rbw;
    vbw = other.vbw;

    auto_rbw = other.auto_rbw;
    auto_vbw = other.auto_vbw;
    native_rbw = other.native_rbw;

    refLevel = other.refLevel;
    div = other.div;
    attenuation = other.attenuation;
    gain = other.gain;
    preamp = other.preamp;

    sweepTime = other.sweepTime;
    processingUnits = other.processingUnits;
    detector = other.detector;
    rejection = other.rejection;

    tgSweepSize = other.tgSweepSize;
    tgHighRangeSweep = other.tgHighRangeSweep;
    tgPassiveDevice = other.tgPassiveDevice;

    // Assuming this is needed for now ?
    UpdateProgram();

    return *this;
}

bool SweepSettings::operator==(const SweepSettings &other) const
{
    if(mode != other.mode) return false;

    if(start != other.start) return false;
    if(stop != other.stop) return false;
    if(center != other.center) return false;
    if(span != other.span) return false;
    if(step != other.step) return false;
    if(rbw != other.rbw) return false;
    if(vbw != other.vbw) return false;

    if(auto_rbw != other.auto_rbw) return false;
    if(auto_vbw != other.auto_vbw) return false;

    if(refLevel != other.refLevel) return false;
    if(div != other.div) return false;
    if(attenuation != other.attenuation) return false;
    if(gain != other.gain) return false;
    if(preamp != other.preamp) return false;

    if(sweepTime != other.sweepTime) return false;
    if(processingUnits != other.processingUnits) return false;
    if(detector != other.detector) return false;
    if(rejection != other.rejection) return false;

    if(tgSweepSize != other.tgSweepSize) return false;
    if(tgHighRangeSweep != other.tgHighRangeSweep) return false;
    if(tgPassiveDevice != other.tgPassiveDevice) return false;

    return true;
}

bool SweepSettings::operator!=(const SweepSettings &other) const
{
    return !(*this == other);
}

// Default values, program launch
void SweepSettings::LoadDefaults()
{
    mode = MODE_SWEEPING;

    std::pair<double, double> freqs = device_traits::full_span_frequencies();
    start = device_traits::best_start_frequency();
    stop = device_traits::max_frequency();

    span = (stop - start);
    center = (start + stop) / 2.0;
    step = 20.0e6;

    auto_rbw = true;
    auto_vbw = true;
    native_rbw = false;

    AutoBandwidthAdjust(true);

    refLevel = Amplitude(-30.0, DBM);
    div = 10.0;
    attenuation = 0;
    gain = 0;
    preamp = 0;

    // Standard sweep only, real-time sweep time in prefs
    sweepTime = 0.001;
    processingUnits = BB_POWER;
    detector = BB_AVERAGE;
    rejection = device_traits::default_spur_reject();

    tgSweepSize = 100;
    tgHighRangeSweep = true;
    tgPassiveDevice = true;

    //emit updated(this);
}

// Preset Load
bool SweepSettings::Load(QSettings &s)
{
    mode = (OperationalMode)(s.value("Mode", (int)Mode()).toInt());

    start = s.value("Sweep/Start", Start().Val()).toDouble();
    stop = s.value("Sweep/Stop", Stop().Val()).toDouble();
    center = s.value("Sweep/Center", Center().Val()).toDouble();
    span = s.value("Sweep/Span", Span().Val()).toDouble();
    step = s.value("Sweep/Step", Step().Val()).toDouble();
    rbw = s.value("Sweep/RBW", RBW().Val()).toDouble();
    vbw = s.value("Sweep/VBW", VBW().Val()).toDouble();

    auto_rbw = s.value("Sweep/AutoRBW", AutoRBW()).toBool();
    auto_vbw = s.value("Sweep/AutoVBW", AutoVBW()).toBool();
    native_rbw = s.value("Sweep/NativeRBW", NativeRBW()).toBool();

    refLevel.Load(s, "Sweep/RefLevel");
    div = s.value("Sweep/Division", Div()).toDouble();
    attenuation = s.value("Sweep/Attenuation", Atten()).toInt();
    gain = s.value("Sweep/Gain", Gain()).toInt();
    preamp = s.value("Sweep/Preamp", Preamp()).toInt();

    sweepTime = s.value("Sweep/SweepTime", SweepTime().Val()).toDouble();
    processingUnits = s.value("Sweep/ProcessingUnits", ProcessingUnits()).toInt();
    detector = s.value("Sweep/Detector", Detector()).toInt();
    rejection = s.value("Sweep/Rejection", Rejection()).toBool();

    tgSweepSize = s.value("Sweep/TgSweepSize", tgSweepSize).toInt();
    tgHighRangeSweep = s.value("Sweep/TgHighRangeSweep", tgHighRangeSweep).toBool();
    tgPassiveDevice = s.value("Sweep/TgPassiveDevice", tgPassiveDevice).toBool();

    UpdateProgram();
    return true;
}

bool SweepSettings::Save(QSettings &s) const
{
    s.setValue("Mode", mode);

    s.setValue("Sweep/Start", start.Val());
    s.setValue("Sweep/Stop", stop.Val());
    s.setValue("Sweep/Center", center.Val());
    s.setValue("Sweep/Span", span.Val());
    s.setValue("Sweep/Step", step.Val());
    s.setValue("Sweep/RBW", rbw.Val());
    s.setValue("Sweep/VBW", vbw.Val());

    s.setValue("Sweep/AutoRBW", auto_rbw);
    s.setValue("Sweep/AutoVBW", auto_vbw);
    s.setValue("Sweep/NativeRBW", native_rbw);

    refLevel.Save(s, "Sweep/RefLevel");
    s.setValue("Sweep/Division", div);
    s.setValue("Sweep/Attenuation", attenuation);
    s.setValue("Sweep/Gain", gain);
    s.setValue("Sweep/Preamp", preamp);

    s.setValue("Sweep/SweepTime", sweepTime.Val());
    s.setValue("Sweep/ProcessingUnits", processingUnits);
    s.setValue("Sweep/Detector", detector);
    s.setValue("Sweep/Rejection", rejection);

    s.setValue("Sweep/TgSweepSize", tgSweepSize);
    s.setValue("Sweep/TgHighRangeSweep", tgHighRangeSweep);
    s.setValue("Sweep/TgPassiveDevice", tgPassiveDevice);

    return true;
}

bool SweepSettings::IsAveragePower() const
{
    return (Detector() == BB_AVERAGE &&
            ProcessingUnits() == BB_POWER);
}

void SweepSettings::AutoBandwidthAdjust(bool force)
{    
    if(Mode() == BB_REAL_TIME) {
        native_rbw = device_traits::has_native_bandwidths();
    }

    if(auto_rbw || force) {
        rbw = device_traits::get_best_rbw(this);
    } else {
        rbw = device_traits::adjust_rbw_on_span(this);
    }

    if(auto_vbw || vbw > rbw || mode == BB_REAL_TIME) {
        vbw = rbw;
    }

    vbw = device_traits::adjust_vbw(this);
    // VBW should not be over 1000 times less than RBW
    if(rbw > vbw * 1000.0) {
        vbw = rbw / 1000.0;
    }

    if(mode == BB_REAL_TIME) {
        double clamped = rbw;
        bb_lib::clamp(clamped, device_traits::min_real_time_rbw(),
                      device_traits::max_real_time_rbw());
        rbw = clamped;
        vbw = rbw;
    }
}

void SweepSettings::UpdateProgram()
{
    if(mode == MODE_NETWORK_ANALYZER) {
        // Minimum 10kHz on fast TG sweep
        if(span > 200.0e3 && start < 10.0e3) {
            start = 10.0e3;
            span = stop - start;
            center = start + (span / 2.0);
        }

        // Minimum 10Hz on slow sweep
        if(span < 200.0e3 && start < 10.0) {
            start = 10.0;
            span = stop - start;
            center = start + (span / 2.0);
        }

        // Must be active device on sweeps below 10kHz
        if(start < 10.0e3) {
            tgPassiveDevice = false;
        }
    }

    emit updated(this);
}

void SweepSettings::setMode(OperationalMode new_mode)
{
    mode = new_mode;
    double maxRealTimeSpan = device_traits::max_real_time_span();

    if(mode == BB_REAL_TIME) {
        native_rbw = device_traits::has_native_bandwidths();
        //auto_rbw = true;
        auto_vbw = true;
        if(bb_lib::clamp(span, Frequency(device_traits::min_real_time_span()),
                         Frequency(device_traits::max_real_time_span()))) {
            start = center - (maxRealTimeSpan / 2.0);
            stop = center + (maxRealTimeSpan / 2.0);
        }

        AutoBandwidthAdjust(/*true*/false);
    }

    UpdateProgram();
}

/*
 * Update start without changing stop, otherwise do nothing
 */
void SweepSettings::setStart(Frequency f)
{   
    bool valid = false;
    Frequency min_start = bb_lib::max2(f.Val(), device_traits::min_frequency());

    // Only change if room
    if(min_start < (stop - device_traits::min_span())) {
        // Special case in real-time
        if(mode == MODE_REAL_TIME) {
            if((stop - min_start) <= device_traits::max_real_time_span() &&
                    (stop - min_start) >= device_traits::min_real_time_span()) {
                valid = true;
            }
        } else { // Other mode
            valid = true;
        }
    }

    if(valid) {
        start = min_start;
        span = stop - start;
        center = start + (span / 2.0);
        //AdjustForSweepSize();
    }

    AutoBandwidthAdjust(false);
    UpdateProgram();
}

/*
 * Update stop without changing start, otherwise to nothing
 */
void SweepSettings::setStop(Frequency f)
{
    bool valid = false;
    double max_freq = device_traits::max_frequency();
    Frequency max_stop = bb_lib::min2(f.Val(), device_traits::max_frequency());

    // Only change if room
    if(max_stop > (start + device_traits::min_span())) {
        if(mode == MODE_REAL_TIME) {
            if((max_stop - start) <= device_traits::max_real_time_span() &&
                    (max_stop - start) >= device_traits::min_real_time_span()) {
                valid = true;
            }
        } else { // other mode
            valid = true;
        }
    }

    if(valid) {
        stop = max_stop;
        span = stop - start;
        center = start + (span / 2.0);
}

    AutoBandwidthAdjust(false);
    UpdateProgram();
}

void SweepSettings::setCenter(Frequency f)
{
    // Is the center even possible?
    if(f < (device_traits::min_real_time_rbw() + device_traits::min_span() * 2.0) ||
            f > (device_traits::max_frequency() - device_traits::min_span() * 2.0)) {
        // Do nothing
    } else {
        center = f;
        span = bb_lib::min3(span.Val(),
                            (center - device_traits::min_frequency()) * 2.0,
                            (device_traits::max_frequency() - center) * 2.0);
        start = center - span / 2.0;
        stop = center + span / 2.0;
    }

    AutoBandwidthAdjust(false);
    UpdateProgram();
}

void SweepSettings::increaseCenter(bool inc)
{
    if(inc) {
        setCenter(center + step);
    } else {
        setCenter(center - step);
    }
}

void SweepSettings::setSpan(Frequency f)
{
    if(f < device_traits::min_span()) {
        f = device_traits::min_span();
    }

    if(Mode() == MODE_REAL_TIME) {
        bb_lib::clamp(f, Frequency(device_traits::min_real_time_span()),
                      Frequency(device_traits::max_real_time_span()));
    }

    // Fit new span to device freq range
    if((center - f / 2.0) < device_traits::min_frequency()) {
        start = device_traits::min_frequency();
        stop = bb_lib::min2((start + f).Val(), device_traits::max_frequency());
    } else if((center + f / 2.0) > device_traits::max_frequency()) {
        stop = device_traits::max_frequency();
        start = bb_lib::max2((stop - f).Val(), device_traits::min_frequency());
    } else {
        start = center - f / 2.0;
        stop = center + f / 2.0;
    }

    center = (start + stop) / 2.0;
    span = stop - start;

    AutoBandwidthAdjust(false);
    UpdateProgram();
}

void SweepSettings::increaseSpan(bool inc)
{
    double new_span = bb_lib::sequence_span(span, inc);
    setSpan(new_span);
}

void SweepSettings::setStep(Frequency f)
{
    step = f;
    UpdateProgram();
}

void SweepSettings::setFullSpan()
{
    std::pair<double, double> freqs = device_traits::full_span_frequencies();
    start = freqs.first;
    stop = freqs.second;
    center = (stop + start) / 2.0;
    span = stop - start;

    auto_rbw = true;
    auto_vbw = true;

    AutoBandwidthAdjust(false);
    UpdateProgram();
}

void SweepSettings::setRBW(Frequency f)
{
    if(native_rbw) {
        int ix = bb_lib::get_native_bw_index(f);
        rbw = native_bw_lut[ix].bw;
    } else {
        rbw = f;
    }

    auto_rbw = false;
    AutoBandwidthAdjust(false);

    UpdateProgram();
}

void SweepSettings::setVBW(Frequency f)
{
    vbw = f;

    auto_vbw = false;
    AutoBandwidthAdjust(false);
    UpdateProgram();
}

void SweepSettings::rbwIncrease(bool inc)
{
    double new_rbw = device_traits::sequence_bw(rbw, native_rbw, inc);

    rbw = new_rbw;
    auto_rbw = false;

    AutoBandwidthAdjust(false);
    UpdateProgram();
}

void SweepSettings::vbwIncrease(bool inc)
{
    double new_vbw = device_traits::sequence_bw(vbw, native_rbw, inc);

    if(new_vbw > rbw) new_vbw = rbw;

    auto_vbw = false;
    vbw = new_vbw;

    AutoBandwidthAdjust(false);
    UpdateProgram();
}

void SweepSettings::setAutoRbw(bool new_auto)
{
    auto_rbw = new_auto;

    AutoBandwidthAdjust(false);
    UpdateProgram();
}

void SweepSettings::setAutoVbw(bool new_auto)
{
    auto_vbw = new_auto;

    if(auto_vbw) {
        vbw = rbw;
    }

    UpdateProgram();
}

void SweepSettings::setNativeRBW(bool native)
{
    native_rbw = native;
    auto_rbw = true;

    AutoBandwidthAdjust(true);
    UpdateProgram();
}

void SweepSettings::setRefLevel(Amplitude new_ref)
{
    if(mode == MODE_NETWORK_ANALYZER) {
        new_ref.Clamp(Amplitude(-130, DBM), Amplitude(40.0, DBM));
    } else {
        new_ref.Clamp(Amplitude(-130, DBM), Amplitude(20.0, DBM));
    }

    refLevel = new_ref;

    UpdateProgram();
}

void SweepSettings::shiftRefLevel(bool inc)
{
    Amplitude newRef;

    if(refLevel.IsLogScale()) {
        if(inc) newRef = refLevel + div;
        else newRef = refLevel - div;
    } else {
        if(inc) newRef = Amplitude(refLevel.Val() * 1.2, AmpUnits::MV);
        else newRef = Amplitude(refLevel.Val() * 0.8, AmpUnits::MV);
    }

    setRefLevel(newRef);
}

void SweepSettings::setDiv(double new_div)
{
    bb_lib::clamp(new_div, 0.1, 30.0);
    div = new_div;

    // Do not update settings if sweeping TG
    if(mode != MODE_NETWORK_ANALYZER) {
        UpdateProgram();
    }
}

/*
 * Store just the index
 * Convert to real atten in device->configure
 */
void SweepSettings::setAttenuation(int atten_ix)
{
    attenuation = atten_ix;
    UpdateProgram();
}

/*
 * Store just the index
 * Convert to real gain in device->configure
 */
void SweepSettings::setGain(int gain_ix)
{
    gain = gain_ix;
    UpdateProgram();
}

void SweepSettings::setPreAmp(int preamp_ix)
{
    preamp = preamp_ix;
    UpdateProgram();
}

void SweepSettings::setDetector(int new_detector)
{
    if(detector != new_detector) {
        detector = new_detector;
        UpdateProgram();
    }
}

void SweepSettings::setProcUnits(int new_units)
{
    if(processingUnits != new_units) {
        processingUnits = new_units;
        UpdateProgram();
    }
}

void SweepSettings::setSweepTime(Time new_sweep_time)
{    
    double st = new_sweep_time.Val();

    if(st < 0.001) st = 0.001;
    if(st > 0.100) st = 0.100;

    sweepTime = st;

    UpdateProgram();
}

void SweepSettings::setRejection(bool image_reject)
{
    if(rejection != image_reject) {
        rejection = image_reject;
        UpdateProgram();
    }
}

void SweepSettings::setTgSweepSize(double sweepSize)
{
    tgSweepSize = (int)sweepSize;
    UpdateProgram();
}

void SweepSettings::setTgHighRange(bool highRange)
{
    tgHighRangeSweep = highRange;
    UpdateProgram();
}

void SweepSettings::setTgPassiveDevice(int isPassive)
{
    if(isPassive == 0) {
        tgPassiveDevice = true;
    } else {
        tgPassiveDevice = false;
    }
    UpdateProgram();
}
