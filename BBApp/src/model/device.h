#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QSize>

#include "lib/bb_lib.h"
#include "lib/bb_api.h"
#include "lib/macros.h"
#include "lib/device_traits.h"

#include "sweep_settings.h"
#include "demod_settings.h"
#include "audio_settings.h"
#include "trace.h"

const int TIMEBASE_INTERNAL = 0;
const int TIMEBASE_EXT_AC = 1;
const int TIMEBASE_EXT_DC = 2;

class Preferences;

enum DeviceSeries {
    saSeries,
    bbSeries
};

// Calibration state regarding the initial store through
enum TgCalState {
    tgCalStateUncalibrated,
    tgCalStatePending,
    tgCalStateCalibrated
};

struct DeviceConnectionInfo {
    int serialNumber;
    DeviceSeries series;
};

// Do not change these values
// They act as indices in the measuring receiver dialog class
enum MeasRcvrRange {
    MeasRcvrRangeHigh = 0,
    MeasRcvrRangeMid = 1,
    MeasRcvrRangeLow = 2
};

class Device : public QObject {
    Q_OBJECT

protected:
    const Preferences *prefs;

public:
    Device(const Preferences *preferences) :
        prefs(preferences)
    {
        current_temp = 0.0;
        voltage = 0.0;
        current = 0.0;
        device_type = DeviceTypeBB60C;
        tgCalState = tgCalStateUncalibrated;
    }
    virtual ~Device() = 0;

    QList<DeviceConnectionInfo> GetDeviceList() const;

    virtual bool OpenDevice() = 0;
    virtual bool OpenDeviceWithSerial(int serialToOpen) = 0;
    virtual int GetNativeDeviceType() const = 0;
    virtual bool CloseDevice() = 0;
    virtual bool Abort() = 0;
    virtual bool Preset() = 0;
    virtual bool Reconfigure(const SweepSettings *s, Trace *t) = 0;
    // Returns false on an unfixable error
    virtual bool GetSweep(const SweepSettings *s, Trace *t) = 0;
    virtual bool GetRealTimeFrame(Trace &t, RealTimeFrame &frame) = 0;
    virtual bool Reconfigure(const DemodSettings *s, IQDescriptor *capture) = 0;
    virtual bool GetIQ(IQCapture *iqc) = 0;
    virtual bool GetIQFlush(IQCapture *iqc, bool flush) = 0;
    virtual bool ConfigureForTRFL(double center, MeasRcvrRange range,
                                  int atten, int gain, IQDescriptor &desc) = 0;
    virtual bool ConfigureAudio(const AudioSettings &as) = 0;
    virtual bool GetAudio(float *audio) = 0;

    virtual bool IsCompatibleWithTg() const { return false; }
    virtual bool AttachTg() { return false; }
    virtual bool IsTgAttached() { return false; }
    virtual bool SetTg(Frequency freq, double amp) { return false; }

    virtual bool CanPerformSelfTest() const { return false; }

    virtual void TgStoreThrough() {}
    virtual void TgStoreThroughPad() {}

    virtual int MsPerIQCapture() const = 0;

    // Only defined for SA devices
    virtual void ConfigureIFOutput(double inputFreq, double outputFreq,
                                   int intputAtten, int outputGain) {}

    // Return the timebase that was set or is going to be set
    virtual int SetTimebase(int new_val) = 0;

    bool IsOpen() const { return open; }
    int Handle() const { return id; }

    virtual QString GetDeviceString() const = 0;
    virtual void UpdateDiagnostics() = 0;

    virtual const char* GetLastStatusString() const = 0;

    DeviceType GetDeviceType() const { return device_type; }
    int SerialNumber() const { return serial_number; }
    QString SerialString() const { return serial_string; }
    QString FirmwareString() const { return firmware_string; }

    float LastConfiguredTemp() const { return last_temp; }
    float CurrentTemp() const { return current_temp; }
    float Voltage() const { return voltage; }
    virtual bool IsPowered() const = 0;
    bool ADCOverflow() const { return adc_overflow; }

    int TimebaseReference() const { return timebase_reference; }
    virtual bool NeedsTempCal() const = 0;
    TgCalState GetTgCalState() const { return tgCalState; }
    QSize RealTimeFrameSize() const { return rtFrameSize; }

protected:
    bool open;
    int id;

    float last_temp; // Temp of last configured state
    float current_temp; // Last retrieved temp
    float voltage;
    float current;

    bool update_diagnostics_string;

    DeviceType device_type;
    int serial_number;
    QString serial_string;
    QString firmware_string;

    int timebase_reference; // Internal/Ext(AC/DC)
    bool reconfigure_on_next; // set true to reconfigure on next sweep

    bool adc_overflow;
    TgCalState tgCalState;
    QSize rtFrameSize;

public slots:

signals:
    void connectionIssues();

private:
    DISALLOW_COPY_AND_ASSIGN(Device)
};

inline Device::~Device() {}

#endif // DEVICE_H
