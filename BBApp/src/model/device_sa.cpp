#include "device_sa.h"
#include "mainwindow.h"
#include "sa_api.h"

#include <QElapsedTimer>

#define STATUS_CHECK(status) \
    if(status < saNoError) { \
    return false; \
    }

DeviceSA::DeviceSA(const Preferences *preferences) :
    Device(preferences)
{
    id = -1;
    open = false;
    serial_number = 0;
    deviceType = saDeviceTypeNone;
    tgIsConnected = false;

    timebase_reference = TIMEBASE_INTERNAL;
    externalReference = false;
}

DeviceSA::~DeviceSA()
{
    CloseDevice();
}

bool DeviceSA::OpenDevice()
{
    if(open) {
        return true;
    }

    lastStatus = saOpenDevice(&id);
    if(lastStatus != saNoError) {
        return false;
    }

    saGetSerialNumber(id, &serial_number);
    serial_string.sprintf("%d", serial_number);
    // Get Firmware version
    char fs[16];
    saGetFirmwareString(id, fs);
    firmware_string = fs;

    saGetDeviceType(id, &deviceType);

    if(deviceType == saDeviceTypeSA44) {
        device_type = DeviceTypeSA44A;
    } else if(deviceType == saDeviceTypeSA44B) {
        device_type = DeviceTypeSA44B;
    } else if(deviceType == saDeviceTypeSA124A || deviceType == saDeviceTypeSA124B) {
        device_type = DeviceTypeSA124;
    }

    saQueryTemperature(id, &current_temp);
    QString diagnostics;
    diagnostics.sprintf("%.2f C", CurrentTemp());
    MainWindow::GetStatusBar()->SetDiagnostics(diagnostics);

    open = true;
    return true;
}

bool DeviceSA::OpenDeviceWithSerial(int serialToOpen)
{
    if(open) {
        return true;
    }

    lastStatus = saOpenDeviceBySerialNumber(&id, serialToOpen);
    if(lastStatus != saNoError) {
        return false;
    }

    saGetSerialNumber(id, &serial_number);
    serial_string.sprintf("%d", serial_number);
    // Get Firmware version
    char fs[16];
    saGetFirmwareString(id, fs);
    firmware_string = fs;
    firmware_string += "  ";

    saGetDeviceType(id, &deviceType);

    if(deviceType == saDeviceTypeSA44) {
        device_type = DeviceTypeSA44A;
    } else if(deviceType == saDeviceTypeSA44B) {
        device_type = DeviceTypeSA44B;
    } else if(deviceType == saDeviceTypeSA124A || deviceType == saDeviceTypeSA124B) {
        device_type = DeviceTypeSA124;
    }

    saQueryTemperature(id, &current_temp);
    QString diagnostics;
    diagnostics.sprintf("%.2f C", CurrentTemp());
    MainWindow::GetStatusBar()->SetDiagnostics(diagnostics);

    open = true;
    return true;
}

int DeviceSA::GetNativeDeviceType() const
{
    if(!open) {
        return (int)saDeviceTypeSA44B;
    }

    saDeviceType type;
    saGetDeviceType(id, &type);
    return (int)type;
}

bool DeviceSA::AttachTg()
{
    SHProgressDialog pd("Looking for Tracking Generator");
    pd.show();

    QEventLoop el;
    std::thread t = std::thread(&DeviceSA::connectTgInThread, this, &el);
    el.exec();
    if(t.joinable()) {
        t.join();
    }

    return tgIsConnected;
}

bool DeviceSA::IsTgAttached()
{

    return false;
}

bool DeviceSA::CloseDevice()
{
    saCloseDevice(id);

    id = -1;
    open = false;
    device_type = DeviceTypeSA44B;
    serial_number = 0;

    return true;
}

bool DeviceSA::Abort()
{
    saAbort(id);
    return true;
}

bool DeviceSA::Preset()
{
    if(!open) {
        //lastStatus = saDeviceNotOpenErr;
        return false;
    }

    saAbort(id);
    saPreset(id);

    return true;
}

bool DeviceSA::Reconfigure(const SweepSettings *s, Trace *t)
{   
    Abort();
    tgCalState = tgCalStateUncalibrated;

    // Update temperature between configurations
    QString diagnostics;
    if(deviceType != saDeviceTypeSA44) {
        saQueryTemperature(id, &current_temp);
        saQueryDiagnostics(id, &voltage);
        diagnostics.sprintf("%.2f C  --  %.2f V", CurrentTemp(), Voltage());
    }
    MainWindow::GetStatusBar()->SetDiagnostics(diagnostics);

    int atten = (s->Atten() == 0) ? SA_AUTO_ATTEN : s->Atten() - 1;
    int gain = (s->Gain() == 0) ? SA_AUTO_GAIN : s->Gain() - 1;
    bool preamp = (s->Preamp() == 2);
    int scale = (s->RefLevel().IsLogScale() ? SA_LOG_SCALE : SA_LIN_SCALE);

    saConfigCenterSpan(id, s->Center(), s->Span());
    saConfigAcquisition(id, s->Detector(), scale);

    if(atten == SA_AUTO_ATTEN || gain == SA_AUTO_GAIN) {
        saConfigLevel(id, s->RefLevel().ConvertToUnits(AmpUnits::DBM));
        saConfigGainAtten(id, SA_AUTO_ATTEN, SA_AUTO_GAIN, true);
    } else {
        saConfigGainAtten(id, atten, gain, preamp);
    }

    saConfigSweepCoupling(id, s->RBW(), s->VBW(), s->Rejection());
    saConfigProcUnits(id, s->ProcessingUnits());

    int init_mode = SA_SWEEPING;
    if(s->Mode() == BB_REAL_TIME) {
        saConfigRealTime(id, s->Div() * 10.0, prefs->realTimeFrameRate);
        init_mode = SA_REAL_TIME;
    }
    if(s->Mode() == MODE_NETWORK_ANALYZER) {
        saConfigTgSweep(id, s->tgSweepSize, s->tgHighRangeSweep, s->tgPassiveDevice);
        init_mode = SA_TG_SWEEP;
    }

    saStatus initStatus = saInitiate(id, init_mode, 0);

    int traceLength = 0;
    double startFreq = 0.0, binSize = 0.0;
    saQuerySweepInfo(id, &traceLength, &startFreq, &binSize);

    t->SetSettings(*s);
    t->SetSize(traceLength);
    t->SetFreq(binSize, startFreq);
    t->SetUpdateRange(0, traceLength);

    if(s->Mode() == MODE_REAL_TIME) {
        int w = 0, h = 0;
        saQueryRealTimeFrameInfo(id, &w, &h);
        rtFrameSize.setWidth(w);
        rtFrameSize.setHeight(h);
    }

    return true;
}

bool DeviceSA::GetSweep(const SweepSettings *s, Trace *t)
{
    saStatus status = saNoError;

    int startIx, stopIx;

    status = saGetPartialSweep_32f(id, t->Min(), t->Max(), &startIx, &stopIx);

    // Testing Full sweep, not for production
    //status = saGetSweep_32f(id, t->Min(), t->Max());
    //startIx = 0;
    //stopIx = t->Length();

    if(status == saUSBCommErr) {
        emit connectionIssues();
        return false;
    }

    t->SetUpdateRange(startIx, stopIx);

    if(s->Mode() == MODE_NETWORK_ANALYZER && tgCalState == tgCalStatePending) {
        if(stopIx >= t->Length()) {
            tgCalState = tgCalStateCalibrated;
        }
    }

    adc_overflow = (status == saCompressionWarning);

    return true;
}

bool DeviceSA::GetRealTimeFrame(Trace &t, RealTimeFrame &frame)
{
    Q_ASSERT(frame.alphaFrame.size() == rtFrameSize.width() * rtFrameSize.height());
    Q_ASSERT(frame.rgbFrame.size() == frame.alphaFrame.size() * 4);

    // TODO check return value, emit error if not good
    saStatus status = saGetRealTimeFrame(id, t.Max(), &frame.alphaFrame[0]);

    if(status == saUSBCommErr) {
        emit connectionIssues();
        return false;
    }

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

    adc_overflow = (status == saCompressionWarning);

    return true;
}

// I/Q streaming setup
bool DeviceSA::Reconfigure(const DemodSettings *s, IQDescriptor *iqc)
{
    saAbort(id);

    int atten = (s->Atten() == 0) ? SA_AUTO_ATTEN : s->Atten() - 1;
    int gain = (s->Gain() == 0) ? SA_AUTO_GAIN : s->Gain() - 1;
    saConfigCenterSpan(id, s->CenterFreq(), 250.0e3);

    if(s->Atten() == 0 || s->Gain() == 0 || s->Preamp() == 0) {
        saConfigLevel(id, s->InputPower());
    } else {
        saConfigGainAtten(id, s->Atten(), s->Gain(), s->Preamp());
    }

    saConfigIQ(id, 0x1 << s->DecimationFactor(), s->Bandwidth());
    saInitiate(id, SA_IQ, 0);

    saQueryStreamInfo(id, &iqc->returnLen, &iqc->bandwidth, &iqc->sampleRate);
    iqc->timeDelta = 1.0 / iqc->sampleRate;
    iqc->decimation = 1;

    return true;
}

bool DeviceSA::GetIQ(IQCapture *iqc)
{
    saStatus status = saGetIQ_32f(id, (float*)(&iqc->capture[0]));

    if(status == saUSBCommErr) {
        emit connectionIssues();
        return false;
    }

    adc_overflow = (status == saCompressionWarning);

    return true;
}

bool DeviceSA::GetIQFlush(IQCapture *iqc, bool sync)
{
    int rs;
    if(!sync) {
        return GetIQ(iqc);
    } else {
        QElapsedTimer timer;
        do {
            timer.start();
            if(!GetIQ(iqc)) {
                return false;
            }
            rs = timer.restart();
            //printf("%d\n", rs);
        } while(rs < 2);
    }
    return true;
}

bool DeviceSA::ConfigureForTRFL(double center,
                                MeasRcvrRange range,
                                int atten,
                                int gain,
                                IQDescriptor &desc)
{
    saAbort(id);

    double refLevel;
    switch(range) {
    case MeasRcvrRangeHigh:
        refLevel = 0.0;
        break;
    case MeasRcvrRangeMid:
        refLevel = -25.0;
        break;
    case MeasRcvrRangeLow:
        refLevel = -50.0;
        break;
    }

    saConfigCenterSpan(id, center, 100.0e3);
    saConfigLevel(id, refLevel);
    saConfigGainAtten(id, SA_AUTO_ATTEN, SA_AUTO_GAIN, false);
    saConfigIQ(id, 2, 100.0e3);
    saInitiate(id, SA_IQ, 0);
    saQueryStreamInfo(id, &desc.returnLen, &desc.bandwidth, &desc.sampleRate);
    desc.timeDelta = 1.0 / desc.sampleRate;
    desc.decimation = 2;

    return true;
}

bool DeviceSA::ConfigureAudio(const AudioSettings &as)
{
    /*lastStatus = */saConfigAudio(
                id,
                as.AudioMode(),
                as.CenterFreq(),
                as.IFBandwidth(),
                as.LowPassFreq(),
                as.HighPassFreq(),
                as.FMDeemphasis());

    /*lastStatus = */saInitiate(id, SA_AUDIO, 0);

    return true;
}

bool DeviceSA::GetAudio(float *audio)
{
    saStatus status = saGetAudio(id, audio);

    if(status == saUSBCommErr) {
        emit connectionIssues();
        return false;
    }

    adc_overflow = (status == saCompressionWarning);

    return true;
}

const char* DeviceSA::GetLastStatusString() const
{
    return saGetErrorString(lastStatus);
}

QString DeviceSA::GetDeviceString() const
{
    if(deviceType == saDeviceTypeSA44) return "SA44";
    if(deviceType == saDeviceTypeSA44B) return "SA44B";
    if(deviceType == saDeviceTypeSA124A) return "SA124A";
    if(deviceType == saDeviceTypeSA124B) return "SA124B";

    return "No Device Open";
}

void DeviceSA::UpdateDiagnostics()
{

}

bool DeviceSA::IsPowered() const
{
    return true;
}

bool DeviceSA::NeedsTempCal() const
{
    return false;
}

bool DeviceSA::SetTg(Frequency freq, double amp)
{
    saStatus stat = saSetTg(id, freq, amp);
    if(stat != saNoError) {
        return false;
    }

    return true;
}

void DeviceSA::TgStoreThrough()
{
    saStoreTgThru(id, TG_THRU_0DB);
    tgCalState = tgCalStatePending;
}

void DeviceSA::TgStoreThroughPad()
{
    saStoreTgThru(id, TG_THRU_20DB);
}

int DeviceSA::SetTimebase(int newTimebase)
{
    if(!externalReference && newTimebase == TIMEBASE_INTERNAL) {
        return timebase_reference;
    }

    if(externalReference) {
        QMessageBox::information(0, "Information",
                                 "Unable to modify the external reference of the device "
                                 "once it has been enabled.");
        return timebase_reference;
    }

    saStatus status = saNoError; // saEnableExternalReference(id);
    if(status == saExternalReferenceNotFound) {
        QMessageBox::warning(0, "Warning",
                             "No external reference on port.\n"
                             "Connect reference and try again.");
        return timebase_reference;
    }

    if(status == saInvalidDeviceErr) {
        QMessageBox::warning(0, "Warning",
                             "The current device does not support\n"
                             "an external reference.");
        return timebase_reference;
    }

    timebase_reference = newTimebase;
    externalReference = true;
    return timebase_reference;
}
void DeviceSA::ConfigureIFOutput(double inputFreq,
                                 double outputFreq,
                                 int inputAtten,
                                 int outputGain)
{
    saConfigIFOutput(id, inputFreq, outputFreq, inputAtten, outputGain);
}
