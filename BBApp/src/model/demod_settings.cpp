#include "demod_settings.h"

DemodSettings::DemodSettings()
{
    LoadDefaults();
}

DemodSettings::DemodSettings(const DemodSettings &other)
{
    *this = other;
}

DemodSettings& DemodSettings::operator=(const DemodSettings &other)
{
    inputPower = other.InputPower();
    centerFreq = other.CenterFreq();
    gain = other.Gain();
    atten = other.Atten();
    decimationFactor = other.DecimationFactor();
    bandwidth = other.Bandwidth();
    autoBandwidth = other.AutoBandwidth();
    sweepTime = other.SweepTime();

    trigType = other.TrigType();
    trigEdge = other.TrigEdge();
    trigAmplitude = other.TrigAmplitude();
    trigPosition = other.TrigPosition();

    maEnabled = other.MAEnabled();
    maLowPass = other.MALowPass();

    return *this;
}

bool DemodSettings::operator==(const DemodSettings &other) const
{
    if(inputPower != other.InputPower()) return false;
    if(centerFreq != other.CenterFreq()) return false;
    if(gain != other.Gain()) return false;
    if(atten != other.Atten()) return false;
    if(decimationFactor != other.DecimationFactor()) return false;
    if(bandwidth != other.Bandwidth()) return false;
    if(autoBandwidth != other.AutoBandwidth()) return false;
    if(sweepTime != other.SweepTime()) return false;

    if(trigType != other.TrigType()) return false;
    if(trigEdge != other.TrigEdge()) return false;
    if(trigAmplitude != other.TrigAmplitude()) return false;
    if(trigPosition != other.TrigPosition()) return false;

    if(maEnabled != other.MAEnabled()) return false;
    if(maLowPass != other.MALowPass()) return false;

    return true;
}

bool DemodSettings::operator!=(const DemodSettings &other) const
{
    return !(*this == other);
}

void DemodSettings::LoadDefaults()
{
    inputPower = 0.0;
    centerFreq = 1.0e9;
    gain = 0; // Index, 0 == auto
    atten = 0; // Index, 0 == auto
    decimationFactor = device_traits::default_decimation();
    bandwidth = device_traits::max_iq_bandwidth(decimationFactor);
    autoBandwidth = true;
    sweepTime = 1.0e-3;

    trigType = TriggerTypeNone;
    trigEdge = TriggerEdgeRising;
    trigAmplitude = 0.0;
    trigPosition = 10.0;

    maEnabled = false;
    maLowPass = 10.0e3;

    emit updated(this);
}

bool DemodSettings::Load(QSettings &s)
{
    inputPower.Load(s, "Demod/InputPower");
    centerFreq = s.value("Demod/CenterFrequency", CenterFreq().Val()).toDouble();
    gain = s.value("Demod/Gain", Gain()).toInt();
    atten = s.value("Demod/Atten", Atten()).toInt();
    decimationFactor = s.value("Demod/Decimation", DecimationFactor()).toInt();
    bandwidth = s.value("Demod/Bandwidth", Bandwidth().Val()).toDouble();
    autoBandwidth = s.value("Demod/AutoBandwidth", AutoBandwidth()).toBool();
    sweepTime = s.value("Demod/SweepTime", SweepTime().Val()).toDouble();

    trigType = (TriggerType)s.value("Demod/TriggerType", TrigType()).toInt();
    trigEdge = (TriggerEdge)s.value("Demod/TriggerEdge", TrigEdge()).toInt();
    trigAmplitude.Load(s, "Demod/TriggerAmplitude");
    trigPosition = s.value("Demod/TriggerPosition", TrigPosition()).toDouble();

    maEnabled = s.value("Demod/MAEnabled", false).toBool();
    maLowPass = s.value("Demod/MALowPass", 10.0e3).toDouble();

    emit updated(this);
    return true;
}

bool DemodSettings::Save(QSettings &s) const
{
    inputPower.Save(s, "Demod/InputPower");
    s.setValue("Demod/CenterFrequency", CenterFreq().Val());
    s.setValue("Demod/Gain", Gain());
    s.setValue("Demod/Atten", Atten());
    s.setValue("Demod/Decimation", DecimationFactor());
    s.setValue("Demod/Bandwidth",Bandwidth().Val());
    s.setValue("Demod/AutoBandwidth", AutoBandwidth());
    s.setValue("Demod/SweepTime", SweepTime().Val());

    s.setValue("Demod/TriggerType", TrigType());
    s.setValue("Demod/TriggerEdge", TrigEdge());
    trigAmplitude.Save(s, "Demod/TriggerAmplitude");
    s.setValue("Demod/TriggerPosition", TrigPosition());

    s.setValue("Demod/MAEnabled", MAEnabled());
    s.setValue("Demod/MALowPass", MALowPass().Val());

    return true;
}

void DemodSettings::setInputPower(Amplitude a)
{
    a.Clamp(Amplitude(-100, DBM), Amplitude(20.0, DBM));
    inputPower = a;
    emit updated(this);
}

void DemodSettings::setCenterFreq(Frequency f)
{
    if(f > Frequency(device_traits::max_frequency())) {
        f = Frequency(device_traits::max_frequency());
    }

    if(f < Frequency(device_traits::min_iq_frequency())) {
        f = Frequency(device_traits::min_iq_frequency());
    }

    centerFreq = f;
    emit updated(this);
}

void DemodSettings::setGain(int g)
{
    if(g < 0) g = 0;
    if(g > device_traits::max_gain()) g = device_traits::max_gain();

    gain = g;
    emit updated(this);
}

void DemodSettings::setAtten(int a)
{
    if(a < 0) a = 0;
    if(a > device_traits::max_atten()) a = device_traits::max_atten();

    atten = a;
    emit updated(this);
}

void DemodSettings::setDecimation(int d)
{
    if(d < 0) d = 0;
    if(d > 7) d = 7;

    decimationFactor = d;

    UpdateAutoBandwidths();
    ClampSweepTime();

    emit updated(this);
}

void DemodSettings::setBandwidth(Frequency bw)
{
    autoBandwidth = false;
    double maxBandwidth = device_traits::max_iq_bandwidth(decimationFactor);

    if(bw < device_traits::min_iq_bandwidth()) {
        bandwidth = device_traits::min_iq_bandwidth();
    } else if(bw > maxBandwidth) {
        bandwidth = maxBandwidth;
    } else {
        bandwidth = bw;
    }

    UpdateAutoBandwidths();
    emit updated(this);
}

void DemodSettings::setAutoBandwidth(bool setAuto)
{
    autoBandwidth = setAuto;
    UpdateAutoBandwidths();
    emit updated(this);
}

// Clamp sweep time to represent a maximum sweep length
void DemodSettings::setSweepTime(Time t)
{
    sweepTime = t;
    ClampSweepTime();

    emit updated(this);
}

void DemodSettings::setTrigType(int tt)
{
    trigType = (TriggerType)tt;
    emit updated(this);
}

void DemodSettings::setTrigEdge(int te)
{
    trigEdge = (TriggerEdge)te;
    emit updated(this);
}

void DemodSettings::setTrigAmplitude(Amplitude ta)
{
    trigAmplitude = ta;
    emit updated(this);
}

void DemodSettings::setTrigPosition(double pos)
{
    bb_lib::clamp(pos, 0.0, 90.0);
    trigPosition = pos;
    emit updated(this);
}

void DemodSettings::setMAEnabled(bool enabled)
{
    maEnabled = enabled;

    if(maEnabled) {
        SetMRConfiguration();
    }

    emit updated(this);
}

void DemodSettings::setMALowPass(Frequency f)
{
    f.Clamp(100.0, 20.0e3);

    maLowPass = f;

    emit updated(this);
}

void DemodSettings::SetMRConfiguration()
{
    // Set proper decimation
    decimationFactor = device_traits::mod_analysis_decimation_order();

    // Force highest bandwidth for given decimation
    double maxBandwidth = device_traits::max_iq_bandwidth(decimationFactor);
    bandwidth = maxBandwidth;

    // Max possible sweep time
    double binSize = (0x1 << decimationFactor) / device_traits::sample_rate();
    sweepTime = binSize * MAX_IQ_SWEEP_LEN;
}

void DemodSettings::ClampSweepTime()
{
    // Max of 1/2 second capture length
    if(sweepTime > 0.5) sweepTime = 0.5;

    // Clamp sweep time to a min/max sweepCount
    double binSize = (0x1 << decimationFactor) / device_traits::sample_rate();
    int sweepLen = sweepTime.Val() / binSize;

    if(sweepLen < MIN_IQ_SWEEP_LEN) {
        sweepTime = binSize * MIN_IQ_SWEEP_LEN;
    } else if(sweepLen > MAX_IQ_SWEEP_LEN) {
        sweepTime = binSize * MAX_IQ_SWEEP_LEN;
    }
}

void DemodSettings::UpdateAutoBandwidths()
{
    double maxBandwidth = device_traits::max_iq_bandwidth(decimationFactor);

    if(autoBandwidth) {
        bandwidth = maxBandwidth;
    } else {
        if(bandwidth > maxBandwidth) {
            bandwidth = maxBandwidth;
        }
    }
}

void IQSweep::Demod()
{
    Q_ASSERT(iq.size() > 0);
    if(iq.size() <= 0) {
        return;
    }

    amWaveform.clear();
    fmWaveform.clear();
    pmWaveform.clear();

    //for(complex_f cplx : iq) {
    for(auto i = 0; i < sweepLen; i++) {
        amWaveform.push_back(iq[i].re * iq[i].re + iq[i].im * iq[i].im);
        pmWaveform.push_back(atan2(iq[i].im, iq[i].re));
    }

    double phaseToFreq = descriptor.sampleRate / BB_TWO_PI;
    double lastPhase = pmWaveform[0];

    for(float phase : pmWaveform) {
        double delPhase = phase - lastPhase;
        lastPhase = phase;

        if(delPhase > BB_PI)
            delPhase -= BB_TWO_PI;
        else if(delPhase < (-BB_PI))
            delPhase += BB_TWO_PI;

        fmWaveform.push_back(delPhase * phaseToFreq);
    }
}

void IQSweep::CalculateReceiverStats()
{
    const std::vector<float> &am = amWaveform;
    const std::vector<float> &fm = fmWaveform;

    // Temp buffers, storing offset removed modulations
    std::vector<float> temp, temp2;
    std::vector<double> audioRate; // Downsampled audio
    int downsampledRate = descriptor.sampleRate / 8;

    temp.resize(fm.size());
    temp2.resize(fm.size());

    // Low pass filter
    FirFilter fir(settings.MALowPass() / descriptor.sampleRate, 1024); // Filters AM and FM

    // Calculate RF Center based on average of FM frequencies
    stats.rfCenter = 0.0;
    for(int i = 2048; i < fm.size(); i++) {
        stats.rfCenter += fm[i];
    }
    double fmAvg = stats.rfCenter / (temp.size() - 2048);
//    qDebug() << "FM Avg " << fmAvg;
    stats.rfCenter = settings.CenterFreq() + fmAvg;

    // Remove DC offset
    for(int i = 0; i < fm.size(); i++) {
        temp[i] = fm[i] - fmAvg;
    }

    fir.Filter(&temp[0], &temp2[0], fm.size());
    fir.Reset();

    stats.fmAudioFreq = getAudioFreq(temp2, descriptor.sampleRate, 1024);

    // FM RMS
    stats.fmPeakPlus = std::numeric_limits<double>::lowest();
    stats.fmPeakMinus = std::numeric_limits<double>::max();
    stats.fmRMS = 0.0;
    for(int i = 1024; i < temp2.size(); i++) {
        if(temp2[i] > stats.fmPeakPlus) stats.fmPeakPlus = temp2[i];
        if(temp2[i] < stats.fmPeakMinus) stats.fmPeakMinus = temp2[i];
        stats.fmRMS += temp2[i] * temp2[i];
    }
    stats.fmRMS = sqrt(stats.fmRMS / (temp2.size() - 1024));

    downsample(temp2, audioRate, 8);
    stats.fmSINAD = 10.0 * log10(CalculateSINAD(audioRate, downsampledRate/*39062.5*/, stats.fmAudioFreq));
    stats.fmTHD = CalculateTHD(audioRate, downsampledRate/*39062.5*/, stats.fmAudioFreq);

    // AM
    double invAvg = 0.0;
    for(int i = 0; i < am.size(); i++) {
        float v = sqrt(am[i] * 50000.0);
        temp[i] = v;
        invAvg += v;
    }
    invAvg = am.size() / invAvg;

    // Normalize around zero
    for(int i = 0; i < temp.size(); i++) {
        temp2[i] = (temp[i] * invAvg) - 1.0;
    }

    fir.Filter(&temp2[0], &temp[0], temp2.size());

    stats.amAudioFreq = getAudioFreq(temp, descriptor.sampleRate, 1024);
    stats.amPeakPlus = std::numeric_limits<double>::lowest();
    stats.amPeakMinus = std::numeric_limits<double>::max();
    stats.amRMS = 0.0;
    for(int i = 1024; i < temp.size(); i++) {
        if(temp[i] > stats.amPeakPlus) stats.amPeakPlus = temp[i];
        if(temp[i] < stats.amPeakMinus) stats.amPeakMinus = temp[i];
        stats.amRMS += (temp[i] * temp[i]);
    }
    stats.amRMS = sqrt(stats.amRMS / (temp.size() - 1024));

    audioRate.clear();
    downsample(temp, audioRate, 8);
    stats.amSINAD = 10.0 * log10(CalculateSINAD(audioRate, downsampledRate/*39062.5*/, stats.amAudioFreq));
    stats.amTHD = CalculateTHD(audioRate, downsampledRate/*39062.5*/, stats.amAudioFreq);
}

// Returns dB ratio of the average power of the waveform over the average
//   power of the waveform with a band reject filter on center freq
double CalculateSINAD(const std::vector<double> &waveform, double sampleRate, double centerFreq)
{
    auto len = waveform.size();
    std::vector<double> rejected;
    std::vector<double> filtered;
    filtered.resize(len);
    rejected.resize(len);

    iirHighPass(&waveform[0], &filtered[0], len);
    iirBandReject(&filtered[0], &rejected[0], centerFreq / sampleRate, 0.005, len);
    //iirBandPass(&waveform[0], &rejected[0], centerFreq / 312.5e3, 0.1, waveform.size());

//    qDebug() << averagePower(&filtered[len/2], len/2);
//    qDebug() << "Rejected " << averagePower(&rejected[len/2], len/2);
    return averagePower(&filtered[len/2], len/2) /
            averagePower(&rejected[len/2], len/2);
}


double CalculateTHD(const std::vector<double> &waveform, double sampleRate, double centerFreq)
{
    const int TOTAL_HARMONICS = 9;
    const double PASS_BAND = 50.0;

    auto len = waveform.size();
    std::vector<double> filtered, filtered2;
    filtered.resize(len);
    filtered2.resize(len);
    double Vharmonics[TOTAL_HARMONICS] = {0};

    for(int h = 0; h < TOTAL_HARMONICS; h++) {
        iirBandPass(&waveform[0], &filtered[0], ((h+1) * centerFreq) / sampleRate,
                PASS_BAND / sampleRate, len);
        iirBandPass(&filtered[0], &filtered2[0], ((h+1) * centerFreq) / sampleRate,
                PASS_BAND / sampleRate, len);
        iirBandPass(&filtered2[0], &filtered[0], ((h+1) * centerFreq) / sampleRate,
                PASS_BAND / sampleRate, len);

        for(int i = 2048; i < len; i++) {
            Vharmonics[h] += (filtered[i] * filtered[i]);
        }
        Vharmonics[h] = sqrt(Vharmonics[h] / (len - 2048));
    }

    double Vrms = 0.0;
    for(int h = 1; h < TOTAL_HARMONICS; h++) {
        Vrms += Vharmonics[h] * Vharmonics[h];
    }
    Vrms = sqrt(Vrms);

//    for(int i = 0; i < TOTAL_HARMONICS; i++) {
//        qDebug() << "V" << QVariant(i+1).toString() << " " << Vharmonics[i];
//    }

    return Vrms / Vharmonics[0];
}
