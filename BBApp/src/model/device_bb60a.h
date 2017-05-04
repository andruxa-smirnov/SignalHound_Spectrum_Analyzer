#ifndef DEVICE_BB60A_H
#define DEVICE_BB60A_H

#include "device.h"

class Preferences;

class DeviceBB60A : public Device {
public:
    DeviceBB60A(const Preferences *preferences);
    virtual ~DeviceBB60A();

    virtual bool OpenDevice();
    virtual bool OpenDeviceWithSerial(int serialToOpen);
    virtual int GetNativeDeviceType() const;
    virtual bool CloseDevice();
    virtual bool Abort();
    virtual bool Preset();
    // Sweep
    virtual bool Reconfigure(const SweepSettings *s, Trace *t);
    virtual bool GetSweep(const SweepSettings *s, Trace *t);
    virtual bool GetRealTimeFrame(Trace &t, RealTimeFrame &frame);
    // Stream
    virtual bool Reconfigure(const DemodSettings *s, IQDescriptor *iqc);
    virtual bool GetIQ(IQCapture *iqc);
    virtual bool GetIQFlush(IQCapture *iqc, bool sync);
    virtual bool ConfigureForTRFL(double center, MeasRcvrRange range,
                                  int atten, int gain, IQDescriptor &desc);
    virtual bool ConfigureAudio(const AudioSettings &as);
    virtual bool GetAudio(float *audio);

    virtual const char* GetLastStatusString() const;

    virtual QString GetDeviceString() const;
    virtual void UpdateDiagnostics();
    virtual bool IsPowered() const;
    virtual bool NeedsTempCal() const { return fabs(last_temp - current_temp) > 2; }

    virtual int MsPerIQCapture() const { return 26; }

    virtual int SetTimebase(int new_val) {
        timebase_reference = new_val;
        reconfigure_on_next = true;
        return timebase_reference;
    }

private:
    // Controls whether or not we need to reinitialize the device when
    //   setting a new audio configuration
    double last_audio_freq;
    int bbDeviceType;
    bbStatus lastStatus;

private:
    DISALLOW_COPY_AND_ASSIGN(DeviceBB60A)
};

#endif // DEVICE_BB60A_H
