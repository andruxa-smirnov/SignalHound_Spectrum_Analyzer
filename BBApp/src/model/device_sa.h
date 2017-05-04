#ifndef DEVICE_SA_H
#define DEVICE_SA_H

#include <QEventLoop>

#include "device.h"
#include "lib/sa_api.h"

class Preferences;

class DeviceSA : public Device {
public:
    DeviceSA(const Preferences *preferences);
    virtual ~DeviceSA();

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
    virtual bool NeedsTempCal() const;

    virtual bool IsCompatibleWithTg() const { return true; }
    virtual bool AttachTg();
    virtual bool IsTgAttached();
    virtual bool SetTg(Frequency freq, double amp);

    virtual bool CanPerformSelfTest() const {
        if(deviceType == saDeviceTypeSA44B
                || deviceType == saDeviceTypeSA124B)
        {
            return true;
        } else {
            return false;
        }
    }

    virtual void TgStoreThrough();
    virtual void TgStoreThroughPad();

    virtual int MsPerIQCapture() const { return 34; }

    virtual int SetTimebase(int);
    virtual void ConfigureIFOutput(double inputFreq, double outputFreq,
                                   int intputAtten, int outputGain);

private:
    void connectTgInThread(QEventLoop *el)
    {
        if(saAttachTg(id) == saNoError) {
            tgIsConnected = true;
        }

        while(!el->isRunning()) { Sleep(1); }
        el->exit();
    }

    saDeviceType deviceType;
    saStatus lastStatus;
    bool tgIsConnected;
    // Once external reference is chosen, unable to return to internal
    bool externalReference;

private:
    DISALLOW_COPY_AND_ASSIGN(DeviceSA)
};

#endif // DEVICE_SA_H
