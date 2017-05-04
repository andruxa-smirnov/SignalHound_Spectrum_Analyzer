#ifndef IF_OUTPUT_DIALOG_H
#define IF_OUTPUT_DIALOG_H

#include <QDialog>

#include "model/device.h"
#include "entry_widgets.h"

class IFOutputDialog : public QDialog {
    Q_OBJECT

public:
    IFOutputDialog(Device *devicePtr, QWidget *parent = 0);
    ~IFOutputDialog();

private:
    void Reconfigure();

    Device *device; // Does not own

    FrequencyEntry *rfInputFreq, *ifOutputFreq;
    NumericEntry *rfInputAtten, *ifOutputGain;
    CheckBoxEntry *injectHighSide;

    Label *rfInputFreqLabel, *ifOutputFreqLabel,
          *rfInputAttenLabel, *ifOutputGainLabel;

    std::pair<Frequency, Frequency> ifOutputRange;

private slots:
    void rfInputFreqChanged(Frequency f)
    {
        if(bb_lib::clamp(f, Frequency(125.0e6), Frequency(13.0e9))) {
            rfInputFreq->SetFrequency(f);
        }
        Reconfigure();
    }

    void ifOutputFreqChanged(Frequency f)
    {
        if(bb_lib::clamp(f, ifOutputRange.first, ifOutputRange.second)) {
            ifOutputFreq->SetFrequency(f);
        }
        Reconfigure();
    }

    void rfInputAttenChanged(double d)
    {
        int atten = (int)d;
        bb_lib::clamp(atten, 0, 30);
        rfInputAtten->SetValue(atten);
        Reconfigure();
    }

    void ifOutputGainChanged(double d)
    {
        int gain = (int)d;
        bb_lib::clamp(gain, 0, 60);
        ifOutputGain->SetValue(gain);
        Reconfigure();
    }

    void injectHighSideChanged(bool)
    {
        Reconfigure();
    }
};

#endif // IF_OUTPUT_DIALOG_H
