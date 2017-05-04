#include "if_output_dialog.h"
#include "lib/sa_api.h"

IFOutputDialog::IFOutputDialog(Device *devicePtr, QWidget *parent) :
    QDialog(parent),
    device(devicePtr)
{
    setWindowTitle("SA124 IF Output");
    setObjectName("SH_Page");
    setFixedSize(500, 150);

    QPoint pos(5, 5);
    QSize entrySize(350, 25);
    QSize labelSize(200, 25);

    rfInputFreq = new FrequencyEntry("RF Input Frequency", 915.0e6, this);
    rfInputFreq->move(pos);
    rfInputFreq->resize(entrySize);
    rfInputFreqLabel = new Label("125 MHz - 12 GHz", this);
    rfInputFreqLabel->move(pos.x() + entrySize.width() + 20, pos.y());
    rfInputFreqLabel->resize(labelSize);
    pos += QPoint(0, rfInputFreq->height());

    // IF Output frequency start value depends on whether the device is a
    //  SA124A or SA124B
    double ifOutputStartFreq = 63.0e6;
    ifOutputRange = std::make_pair(Frequency(61.0e6), Frequency(65.0e6));
    if(device->GetNativeDeviceType() == saDeviceTypeSA124A) {
        ifOutputStartFreq = 36.0e6;
        ifOutputRange = std::make_pair(Frequency(34.0e6), Frequency(38.0e6));
    }

    ifOutputFreq = new FrequencyEntry("IF Output Frequency", ifOutputStartFreq, this);
    ifOutputFreq->move(pos);
    ifOutputFreq->resize(entrySize);
    ifOutputFreqLabel = new Label(ifOutputRange.first.GetFreqString(0, true) + " - " +
                                  ifOutputRange.second.GetFreqString(0, true), this);
    ifOutputFreqLabel->move(pos.x() + entrySize.width() + 20, pos.y());
    ifOutputFreqLabel->resize(labelSize);
    pos += QPoint(0, ifOutputFreq->height());

    rfInputAtten = new NumericEntry("RF Input Atten", 20, "dB", this);
    rfInputAtten->move(pos);
    rfInputAtten->resize(entrySize);
    rfInputAttenLabel = new Label("0 - 30 dB", this);
    rfInputAttenLabel->move(pos.x() + entrySize.width() + 20, pos.y());
    rfInputAttenLabel->resize(labelSize);
    pos += QPoint(0, rfInputAtten->height());

    ifOutputGain = new NumericEntry("IF Output Gain", 10, "dB", this);
    ifOutputGain->move(pos);
    ifOutputGain->resize(entrySize);
    ifOutputGainLabel = new Label("0 - 60 dB", this);
    ifOutputGainLabel->move(pos.x() + entrySize.width() + 20, pos.y());
    ifOutputGainLabel->resize(labelSize);
    pos += QPoint(0, ifOutputGain->height());

    injectHighSide = new CheckBoxEntry("Inject High-Side", this);
    injectHighSide->move(pos);
    injectHighSide->resize(entrySize);
    pos += QPoint(0, injectHighSide->height());

    connect(rfInputFreq, SIGNAL(freqViewChanged(Frequency)),
            this, SLOT(rfInputFreqChanged(Frequency)));
    connect(ifOutputFreq, SIGNAL(freqViewChanged(Frequency)),
            this, SLOT(ifOutputFreqChanged(Frequency)));
    connect(rfInputAtten, SIGNAL(valueChanged(double)),
            this, SLOT(rfInputAttenChanged(double)));
    connect(ifOutputGain, SIGNAL(valueChanged(double)),
            this, SLOT(ifOutputGainChanged(double)));
    connect(injectHighSide, SIGNAL(clicked(bool)),
            this, SLOT(injectHighSideChanged(bool)));

    Reconfigure();
}

IFOutputDialog::~IFOutputDialog()
{

}

void IFOutputDialog::Reconfigure()
{
    double IFFreq = ifOutputFreq->GetFrequency();
    if(injectHighSide->IsChecked()) {
        IFFreq = -IFFreq;
    }

    device->ConfigureIFOutput(rfInputFreq->GetFrequency(),
                              IFFreq,
                              (int)rfInputAtten->GetValue(),
                              (int)ifOutputGain->GetValue());
}
