#include "device_bb60a.h"
#include "mainwindow.h"

#include <QElapsedTimer>

#define STATUS_CHECK(status) \
    lastStatus = status; \
    if(lastStatus < bbNoError) { \
        return false; \
    } /* end */

DeviceBB60A::DeviceBB60A(const Preferences *preferences) :
    Device(preferences)
{
    id = -1;
    open = false;
    serial_number = 0;
    lastStatus = bbNoError;

    timebase_reference = TIMEBASE_INTERNAL;

    last_audio_freq = -1.0e6;
}

DeviceBB60A::~DeviceBB60A()
{
    CloseDevice();
}

bool DeviceBB60A::OpenDevice()
{
    if(open) {
        lastStatus = bbNoError;
        return true;
    }

    STATUS_CHECK(bbOpenDevice(&id));

    bbGetSerialNumber(id, (unsigned int*)(&serial_number));
    serial_string.sprintf("%d", serial_number);
    int fv;
    bbGetFirmwareVersion(id, &fv);
    firmware_string.sprintf("%d", fv);

    bbGetDeviceType(id, &bbDeviceType);

    if(bbDeviceType == BB_DEVICE_BB60A) {
        device_type = DeviceTypeBB60A;
    } else if(bbDeviceType == BB_DEVICE_BB60C) {
        device_type = DeviceTypeBB60C;
    }

    open = true;
    return true;
}

bool DeviceBB60A::OpenDeviceWithSerial(int serialToOpen)
{
    if(open) {
        lastStatus = bbNoError;
        return true;
    }

    STATUS_CHECK(bbOpenDeviceBySerialNumber(&id, serialToOpen));

    bbGetSerialNumber(id, (unsigned int*)(&serial_number));
    serial_string.sprintf("%d", serial_number);
    int fv;
    bbGetFirmwareVersion(id, &fv);
    firmware_string.sprintf("%d  ", fv);

    bbGetDeviceType(id, &bbDeviceType);

    if(bbDeviceType == BB_DEVICE_BB60A) {
        device_type = DeviceTypeBB60A;
    } else if(bbDeviceType == BB_DEVICE_BB60C) {
        device_type = DeviceTypeBB60C;
    }

    open = true;
    return true;
}

int DeviceBB60A::GetNativeDeviceType() const
{
    if(!open) {
        return BB_DEVICE_BB60C;
    }

    int type;
    bbGetDeviceType(id, &type);
    return type;
}

bool DeviceBB60A::CloseDevice()
{
    if(!open) {
        lastStatus = bbDeviceNotOpenErr;
        return false;
    }

    bbCloseDevice(id);

    id = -1;
    open = false;
    device_type = DeviceTypeBB60C;
    serial_number = 0;

    return true;
}

bool DeviceBB60A::Abort()
{
    if(!open) {
        lastStatus = bbDeviceNotOpenErr;
        return false;
    }

    STATUS_CHECK(bbAbort(id));
    return true;
}

bool DeviceBB60A::Preset()
{
    if(!open) {
        lastStatus = bbDeviceNotOpenErr;
        return false;
    }

    bbAbort(id);
    bbPreset(id);

    // Need to call bbCloseDevice for BB60A/C
    CloseDevice();

    SleepEvent preset_wait;
    preset_wait.Sleep(3000);

    return true;
}

bool DeviceBB60A::Reconfigure(const SweepSettings *s, Trace *t)
{
    Abort();

    int scale = s->RefLevel().IsLogScale() ?
                BB_LOG_SCALE : BB_LIN_SCALE;
    int detector = (s->Detector() == BB_AVERAGE) ?
                BB_AVERAGE : BB_MIN_AND_MAX;
    int rbw_type = (s->NativeRBW()) ?
                BB_NATIVE_RBW : BB_NON_NATIVE_RBW;
    int rejection = (s->Rejection()) ?
                BB_SPUR_REJECT : BB_NO_SPUR_REJECT;
    double sweep_time = s->SweepTime().Val();

    double reference = s->RefLevel().ConvertToUnits(AmpUnits::DBM);

    int port_one_mask;
    switch(timebase_reference) {
    case TIMEBASE_INTERNAL:
        port_one_mask = 0x0;
        break;
    case TIMEBASE_EXT_AC:
        port_one_mask = BB_PORT1_EXT_REF_IN | BB_PORT1_AC_COUPLED;
        break;
    case TIMEBASE_EXT_DC:
        port_one_mask = BB_PORT1_EXT_REF_IN | BB_PORT1_DC_COUPLED;
        break;
    }

    int mode;
    switch(s->Mode()) {
    case MODE_SWEEPING: case MODE_HARMONICS:
        mode = BB_SWEEPING;
        break;
    case MODE_REAL_TIME:
        bbConfigureRealTime(id, s->Div() * 10.0, prefs->realTimeFrameRate);
        mode = BB_REAL_TIME;
        break;
    default:
        Q_ASSERT(0);
        return false;
    }

    STATUS_CHECK(bbConfigureIO(id, port_one_mask, 0x0));

    // Scale based on amp unit type
    STATUS_CHECK(bbConfigureAcquisition(id, detector, scale));
    STATUS_CHECK(bbConfigureCenterSpan(id, s->Center(), s->Span()));
    STATUS_CHECK(bbConfigureLevel(id, reference, (s->Atten()-1) * 10));
    STATUS_CHECK(bbConfigureSweepCoupling(id, s->RBW(), s->VBW(), sweep_time,
                                          rbw_type, rejection));
    STATUS_CHECK(bbConfigureWindow(id, BB_NUTALL));
    STATUS_CHECK(bbConfigureProcUnits(id, s->ProcessingUnits()));
    STATUS_CHECK(bbConfigureGain(id, s->Gain() - 1));

    STATUS_CHECK(bbInitiate(id, mode, 0));

    unsigned int traceSize;
    double binSize, startFreq;
    STATUS_CHECK(bbQueryTraceInfo(id, &traceSize, &binSize, &startFreq));

    t->SetSettings(*s);
    t->SetSize(traceSize);
    t->SetFreq(binSize, startFreq);
    t->SetUpdateRange(0, traceSize);

    if(s->Mode() == MODE_REAL_TIME) {
        int w = 0, h = 0;
        bbQueryRealTimeInfo(id, &w, &h);
        rtFrameSize.setWidth(w);
        rtFrameSize.setHeight(h);
    }

    bbGetDeviceDiagnostics(id, &last_temp, &voltage, &current);

    return true;
}

bool DeviceBB60A::GetSweep(const SweepSettings *s, Trace *t)
{
    // Get new diagnostic info, determine whether to update the
    //  diagnostic string
    UpdateDiagnostics();

    // Print new diagnostics only when a value changed
    if(update_diagnostics_string) {
        QString diagnostics;
        diagnostics.sprintf("%.2f C  --  %.2f V", CurrentTemp(), Voltage());
        MainWindow::GetStatusBar()->SetDiagnostics(diagnostics);
        update_diagnostics_string = false;
    }

    if((fabs(current_temp - last_temp) > 2.0) || reconfigure_on_next) {
        Reconfigure(s, t);
        reconfigure_on_next = false;
    }

    // Manually handle some errors, and populate variables in the event of warnings
    lastStatus = bbFetchTrace_32f(id, t->Length(), t->Min(), t->Max());
    if(lastStatus == bbDeviceConnectionErr) {
        // True connection issue, throw signal
        // emit connectionIssue() throw warning
        emit connectionIssues();
        return false;
    } else if(lastStatus == bbUSBTimeoutErr) {
        // In the event of a timeout, and the device is in a sweep mode, try one more time
        if(s->Mode() == BB_SWEEPING) {
            lastStatus = bbFetchTrace_32f(id, t->Length(), t->Min(), t->Max());
            if(lastStatus < bbNoError) {
                // Second error in a row, disconnect device with issues
                emit connectionIssues();
                return false;
            }
        }
    }

    // After error checks, do warning checks
    if(lastStatus == bbADCOverflow) {
        adc_overflow = true;
    } else {
        adc_overflow = false;
    }

    return true;
}

bool DeviceBB60A::GetRealTimeFrame(Trace &t, RealTimeFrame &frame)
{
    Q_ASSERT(frame.alphaFrame.size() == rtFrameSize.width() * rtFrameSize.height());
    Q_ASSERT(frame.rgbFrame.size() == frame.alphaFrame.size() * 4);

    // Get new diagnostic info, determine whether to update the
    //  diagnostic string
    UpdateDiagnostics();

    // Print new diagnostics only when a value changed
    if(update_diagnostics_string) {
        QString diagnostics;
        diagnostics.sprintf("%.2f C  --  %.2f V", CurrentTemp(), Voltage());
        MainWindow::GetStatusBar()->SetDiagnostics(diagnostics);
        update_diagnostics_string = false;
    }

    lastStatus = bbFetchRealTimeFrame(id, t.Max(), &frame.alphaFrame[0]);
    if(lastStatus == bbDeviceConnectionErr || lastStatus == bbUSBTimeoutErr) {
        emit connectionIssues();
        return false;
    }

    adc_overflow = (lastStatus == bbADCOverflow);

    // Real-time only returns a max or avg trace
    // Copy max into min for real-time
    for(int i = 0; i < t.Length(); i++) {
        t.Min()[i] = t.Max()[i];
    }

    // Convert the alpha/intensity frame to a 4 channel image
    int totalPixels = frame.dim.height() * frame.dim.width();
    for(int i = 0; i < totalPixels; i++) {
        int toSet = frame.alphaFrame[i] * 255;
        if(toSet > 255) toSet = 255;
        frame.rgbFrame[i*4] = frame.rgbFrame[i*4+1] = frame.rgbFrame[i*4+2] = toSet;
        if(frame.alphaFrame[i] > 0.0) {
            frame.rgbFrame[i*4+3] = 255;
        } else {
            frame.rgbFrame[i*4+3] = 0;
        }
    }

    return true;
}

bool DeviceBB60A::Reconfigure(const DemodSettings *ds, IQDescriptor *desc)
{   
    Abort();

    int gain = ds->Gain() - 1, atten = (ds->Atten() - 1) * 10.0;
    if(gain < 0) gain = BB_AUTO_GAIN;
    if(atten < 0) atten = BB_AUTO_ATTEN;
    int decimation = 0x1 << (ds->DecimationFactor());

    int port_one_mask;
    switch(timebase_reference) {
    case TIMEBASE_INTERNAL:
        port_one_mask = 0x0;
        break;
    case TIMEBASE_EXT_AC:
        port_one_mask = BB_PORT1_EXT_REF_IN | BB_PORT1_AC_COUPLED;
        break;
    case TIMEBASE_EXT_DC:
        port_one_mask = BB_PORT1_EXT_REF_IN | BB_PORT1_DC_COUPLED;
        break;
    }

    int port_two_mask;
    switch(ds->TrigType()) {
    case TriggerTypeExternal:
        if(ds->TrigEdge() == TriggerEdgeRising) {
            port_two_mask = BB_PORT2_IN_TRIGGER_RISING_EDGE;
        } else {
            port_two_mask = BB_PORT2_IN_TRIGGER_FALLING_EDGE;
        }
        break;
    default:
        port_two_mask = 0x0;
    }

    bbConfigureIO(id, port_one_mask, port_two_mask);
    bbConfigureCenterSpan(id, ds->CenterFreq(), 1.0e6);
    bbConfigureIQ(id, decimation, ds->Bandwidth());
    bbConfigureLevel(id, ds->InputPower().ConvertToUnits(DBM), atten);
    bbConfigureGain(id, gain);

    bbInitiate(id, BB_STREAMING, BB_STREAM_IQ);

    int sampleRate;
    bbQueryStreamInfo(id, &desc->returnLen, &desc->bandwidth, &sampleRate);
    desc->sampleRate = (double)sampleRate;
    desc->timeDelta = 1.0 / (double)desc->sampleRate;
    desc->decimation = decimation;

    return true;
}

bool DeviceBB60A::GetIQ(IQCapture *iqc)
{
    lastStatus = bbFetchRaw(id, (float*)(&iqc->capture[0]), iqc->triggers);
    // Handle connection issues
    if(lastStatus == bbDeviceConnectionErr || lastStatus == bbUSBTimeoutErr || lastStatus == bbPacketFramingErr) {
        emit connectionIssues();
        return false;
    }
    adc_overflow = (lastStatus == bbADCOverflow);

    return true;
}

// Keep getting IQ data while the device returns immediately.
// We want to remove any queued data
bool DeviceBB60A::GetIQFlush(IQCapture *iqc, bool flush)
{
    if(!flush) {
        return GetIQ(iqc);
    } else {
        QElapsedTimer timer;
        do {
            timer.start();
            if(!GetIQ(iqc)) {
                return false;
            }
        } while(timer.restart() < 2);
    }
    return true;
}

// Tuned RF Level measurements configuration
// Requires lowest sample rate, smallest bandwidth streaming
bool DeviceBB60A::ConfigureForTRFL(double center,
                                   MeasRcvrRange range,
                                   int atten,
                                   int gain,
                                   IQDescriptor &desc)
{
    Abort();

    int port_one_mask;
    switch(timebase_reference) {
    case TIMEBASE_INTERNAL:
        port_one_mask = 0x0;
        break;
    case TIMEBASE_EXT_AC:
        port_one_mask = BB_PORT1_EXT_REF_IN | BB_PORT1_AC_COUPLED;
        break;
    case TIMEBASE_EXT_DC:
        port_one_mask = BB_PORT1_EXT_REF_IN | BB_PORT1_DC_COUPLED;
        break;
    }

    double refLevel;
    switch(range) {
    case MeasRcvrRangeHigh:
        refLevel = +10.0;
        break;
    case MeasRcvrRangeMid:
        refLevel = -25.0;
        break;
    case MeasRcvrRangeLow:
        refLevel = -50.0;
        break;
    }

    bbConfigureIO(id, port_one_mask, 0x0);
    bbConfigureCenterSpan(id, center, 20.0e6);
    bbConfigureIQ(id, 128, 100.0e3);
    bbConfigureLevel(id, refLevel, BB_AUTO_ATTEN);
    bbConfigureGain(id, BB_AUTO_GAIN);

    bbInitiate(id, BB_STREAMING, BB_STREAM_IQ);
    int sampleRate;
    bbQueryStreamInfo(id, &desc.returnLen, &desc.bandwidth, &sampleRate);
    desc.sampleRate = (double)sampleRate;
    desc.timeDelta = 1.0 / (double)desc.sampleRate;
    desc.decimation = 128;

    return true;
}

bool DeviceBB60A::ConfigureAudio(const AudioSettings &as)
{
    lastStatus = bbConfigureDemod(
                id,
                as.AudioMode(),
                as.CenterFreq(),
                as.IFBandwidth(),
                as.LowPassFreq(),
                as.HighPassFreq(),
                as.FMDeemphasis());

    double low_limit = last_audio_freq - 1.75e6;
    double high_limit = last_audio_freq + 1.75e6;

    if(as.CenterFreq() < low_limit || as.CenterFreq() > high_limit) {
        lastStatus = bbInitiate(id, BB_AUDIO_DEMOD, 0);
        last_audio_freq = as.CenterFreq();
    }

    return true;
}

bool DeviceBB60A::GetAudio(float *audio)
{
    lastStatus = bbFetchAudio(id, audio);

    return true;
}

void DeviceBB60A::UpdateDiagnostics()
{
    float temp_now, voltage_now, current_now;
    bbGetDeviceDiagnostics(id, &temp_now, &voltage_now, &current_now);

    if((temp_now != current_temp) || (voltage_now != voltage) || (current_now != current)) {
        update_diagnostics_string = true;
    }

    current_temp = temp_now;
    voltage = voltage_now;
    current = current_now;
}

const char* DeviceBB60A::GetLastStatusString() const
{
    return bbGetErrorString(lastStatus);
}

QString DeviceBB60A::GetDeviceString() const {
    if(bbDeviceType == BB_DEVICE_NONE) return "No Device Open";
    if(bbDeviceType == BB_DEVICE_BB60A) return "BB60A";
    if(bbDeviceType == BB_DEVICE_BB60C) return "BB60C";
    return "No Device Open";
}

bool DeviceBB60A::IsPowered() const
{
    if(device_type == BB_DEVICE_BB60A) {
        if(voltage < 4.4) {
            return false;
        }
    } else if(device_type == BB_DEVICE_BB60C) {
        if(voltage < 4.45) {
            return false;
        }
    }

    return true;
}
