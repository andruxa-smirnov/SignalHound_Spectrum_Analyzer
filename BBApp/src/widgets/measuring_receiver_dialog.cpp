#include "measuring_receiver_dialog.h"

#include <QRadioButton>
#include <QButtonGroup>

#include <iostream>

static const int WIDGET_HEIGHT = 25;

MeasuringReceiver::MeasuringReceiver(Device *devicePtr,
                                     double initialCenter,
                                     QWidget *parent) :
    QDialog(parent),
    device(devicePtr),
    running(true),
    reinitialize(true),
    recalibrate(true)
{
    setWindowTitle("Measuring Receiver");
    setObjectName("SH_Page");
    setFixedWidth(400);

    QPoint pos(0, 0); // For positioning widgets

    Label *title = new Label("Synchronous Level Detector", this);
    title->move(QPoint(5, pos.y()));
    title->resize(width(), WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT+5);
    freqEntry = new FrequencyEntry("Center Freq", initialCenter, this);
    freqEntry->move(pos);
    freqEntry->resize(width() * 0.75, WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT+5);

    Label *ampRangeLabel = new Label("Amplitude Range", this);
    ampRangeLabel->move(QPoint(5, pos.y()));
    ampRangeLabel->resize(width(), WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT);

    ampGroup = new QButtonGroup(this);
    QRadioButton *highAmp = new QRadioButton("High Power Range", this),
            *midAmp = new QRadioButton("Mid Power Range", this),
            *lowAmp = new QRadioButton("Low Power Range", this);

    highAmp->move(pos);
    highAmp->resize(width()/2, WIDGET_HEIGHT);
    highAmp->setObjectName("SHPrefRadioButton");
    highAmp->setChecked(true);
    ampGroup->addButton(highAmp, MeasRcvrRangeHigh);

    highLabel = new Label("", this);
    highLabel->move(QPoint(width()/2, pos.y()));
    highLabel->resize(width()/2, WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT);

    midAmp->move(pos);
    midAmp->resize(width()/2, WIDGET_HEIGHT);
    midAmp->setObjectName("SHPrefRadioButton");
    ampGroup->addButton(midAmp, MeasRcvrRangeMid);

    midLabel = new Label("", this);
    midLabel->move(QPoint(width()/2, pos.y()));
    midLabel->resize(width()/2, WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT);

    lowAmp->move(pos);
    lowAmp->resize(width()/2, WIDGET_HEIGHT);
    lowAmp->setObjectName("SHPrefRadioButton");
    ampGroup->addButton(lowAmp, MeasRcvrRangeLow);

    lowLabel = new Label("", this);
    lowLabel->move(QPoint(width()/2, pos.y()));
    lowLabel->resize(width()/2, WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT+5);

    Label *centerLabel = new Label("RF Frequency", this);
    centerLabel->move(QPoint(5, pos.y()));
    centerLabel->resize(width()/2, WIDGET_HEIGHT);
    centerReadout = new Label("915.11002 MHz", this);
    centerReadout->move(QPoint(width()/2 + 5, pos.y()));
    centerReadout->resize(width()/2, WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT);

    Label *powerLabel = new Label("RF Power", this);
    powerLabel->move(QPoint(5, pos.y()));
    powerLabel->resize(width()/2, WIDGET_HEIGHT);
    powerReadout = new Label("-32.22 dBm", this);
    powerReadout->move(QPoint(width()/2 + 5, pos.y()));
    powerReadout->resize(width()/2, WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT);

    Label *relativeLabel = new Label("Relative Power", this);
    relativeLabel->move(QPoint(5, pos.y()));
    relativeLabel->resize(width()/2, WIDGET_HEIGHT);
    relativeReadout = new Label("-5.002 dB", this);
    relativeReadout->move(QPoint(width()/2 + 5, pos.y()));
    relativeReadout->resize(width()/2, WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT);

    Label *averageLabel = new Label("Average Relative Power", this);
    averageLabel->move(QPoint(5, pos.y()));
    averageLabel->resize(width()/2, WIDGET_HEIGHT);
    averageReadout = new Label("-4.998 dB", this);
    averageReadout->move(QPoint(width()/2 + 5, pos.y()));
    averageReadout->resize(width()/2, WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT*1.5);

    SHPushButton *sync = new SHPushButton("Sync", this);
    sync->move(QPoint(5, pos.y()));
    sync->resize(width()/2 - 10, WIDGET_HEIGHT);
    //pos += QPoint(0, WIDGET_HEIGHT);

    SHPushButton *done = new SHPushButton("Done", this);
    done->move(QPoint(width()/2 + 5, pos.y()));
    done->resize(width()/2 - 10, WIDGET_HEIGHT);
    pos += QPoint(0, WIDGET_HEIGHT*2);

    setFixedHeight(pos.y());
    setSizeGripEnabled(false);

    connect(sync, SIGNAL(clicked()), this, SLOT(triggerReinitialize()));
    connect(done, SIGNAL(clicked()), this, SLOT(accept()));

    connect(freqEntry, SIGNAL(freqViewChanged(Frequency)),
            this, SLOT(triggerReinitialize()));

    connect(this, SIGNAL(updateLabels(QString,QString,QString,QString)),
            this, SLOT(setLabels(QString,QString,QString,QString)));
    connect(this, SIGNAL(updateRangeLevelText(QString, int)),
            this, SLOT(setRangeLevelText(QString, int)));
    connect(this, SIGNAL(updateRangeLevelText(int,QString,int)),
            this, SLOT(setRangeLevelText(int,QString,int)));
    connect(this, SIGNAL(updateRangeEnabled(int)),
            this, SLOT(setRangeEnabled(int)));
    connect(this, SIGNAL(updateEntryEnabled(bool)),
            this, SLOT(setEntryEnabled(bool)));

    connect(ampGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(triggerRecalibration()));

    threadHandle = std::thread(&MeasuringReceiver::ProcessThread, this);
}

MeasuringReceiver::~MeasuringReceiver()
{
    running = false;
    if(threadHandle.joinable()) {
        threadHandle.join();
    }
}

void MeasuringReceiver::Recalibrate(double &centerOut, double &powerOut, IQCapture &iqc)
{
    int atten, gain;
    MeasRcvrRange level = GetCurrentRange();

    switch(level) {
    case MeasRcvrRangeHigh: atten = 30; gain = 1; break;
    case MeasRcvrRangeMid: atten = 30; gain = 3; break;
    case MeasRcvrRangeLow: atten = 0; gain = 3; break;
    }

    emit updateLabels("", "", "", "");
    emit updateRangeLevelText("Calibrating", rangeColorGreen);
    emit updateRangeEnabled(rangeLevelNone);
    emit updateEntryEnabled(false);

    device->ConfigureForTRFL(freqEntry->GetFrequency(), level, atten, gain, descriptor);
    iqc.capture.resize(descriptor.returnLen);
    for(int i = 0; i < 3; i++) {
        device->GetIQFlush(&iqc, true);
    }
    centerOut = getSignalFrequency(iqc.capture, descriptor.sampleRate); // 312500.0);
    centerOut /= descriptor.sampleRate; //312500.0;
    for(int i = 0; i < 30; i++) { // Amplitude settles in roughly 30 packets ?
        if(i % 10 == 0) {
            QString calStr = "Calibrating";
            for(int j = 0; j < (i / 10) + 1; j++) calStr += ".";
            emit updateRangeLevelText(calStr, rangeColorGreen);
        }
        device->GetIQFlush(&iqc, true);
        getPeakCorrelation(&iqc.capture[0], 4096, centerOut,
                centerOut, powerOut, descriptor.sampleRate);
        std::cout << "Pre Center " << centerOut << " " << powerOut << std::endl;
    }

    emit updateRangeLevelText("", rangeColorBlack);
    emit updateEntryEnabled(true);
    recalibrate = false;
}

void MeasuringReceiver::ProcessThread()
{
    IQCapture capture;
    double center, power, relative, offset;
    std::list<double> average;
    std::vector<complex_f> full;

    while(running) {
        // Reinitialize to start tuned level measurements over
        if(reinitialize) {
            ampGroup->button(0)->setChecked(true); // Select high power
            emit updateRangeEnabled(rangeLevelNone);
            power = 0.0;
            offset = 0.0;
            relative = 0.0;
            reinitialize = false;
            recalibrate = true;
        }

        // Recalibrates to a new range
        // Range must be chosen before entering this block
        if(recalibrate) {
            offset += (power - relative);
            Recalibrate(center, power, capture);
            relative = power;
            full.resize(descriptor.returnLen * 3);
            average.clear();
        }

        device->GetIQFlush(&capture, true);
        simdCopy_32fc(&capture.capture[0], &full[0], descriptor.returnLen);
        for(int i = 1; i < 3; i++) {
            device->GetIQ(&capture);
            simdCopy_32fc(&capture.capture[0], &full[i * descriptor.returnLen], descriptor.returnLen);
        }
        getPeakCorrelation(&full[0], descriptor.returnLen*3, center, center, power, descriptor.sampleRate);
        std::cout << "Post center " << center << " " << power << "\n";

        if(device->ADCOverflow()) {
            emit updateRangeLevelText("IF Overload", rangeColorRed);
        } else {
            if(GetCurrentRange() == MeasRcvrRangeHigh) {
                if(power > -25.0) {
                    emit updateRangeLevelText("", rangeColorBlack);
                    emit updateRangeEnabled(rangeLevelNone);
                } else if(power < -40.0) {
                    emit updateRangeLevelText(MeasRcvrRangeMid, "Passed Mid-Range", rangeColorRed);
                    emit updateRangeEnabled(rangeLevelNone);
                } else {
                    emit updateRangeLevelText(MeasRcvrRangeMid, "Recal at new range", rangeColorOrange);
                    emit updateRangeEnabled(MeasRcvrRangeMid);
                }
            } else if(GetCurrentRange() == MeasRcvrRangeMid) {
                if(power > -50.0) {
                    emit updateRangeLevelText("", rangeColorBlack);
                    emit updateRangeEnabled(rangeLevelNone);
                } else if(power < -65.0) {
                    emit updateRangeLevelText(MeasRcvrRangeLow, "Passed Low-Range", rangeColorRed);
                    emit updateRangeEnabled(rangeLevelNone);
                } else {
                    emit updateRangeLevelText(MeasRcvrRangeLow, "Recal at new range", rangeColorOrange);
                    emit updateRangeEnabled(MeasRcvrRangeLow);
                }
            } else {
                emit updateRangeLevelText("", rangeColorBlack);
                emit updateRangeEnabled(rangeLevelNone);
            }
        }

        // Lets update our text
        double diff = (power - relative) + offset;
        average.push_front(diff);
        while(average.size() > 20) average.pop_back();
        double avgPower = 0.0;
        for(double d : average) avgPower += d;
        avgPower /= average.size();

        QString centerStr, powerStr, relativeStr, averageStr;
        centerStr = Frequency(freqEntry->GetFrequency() +
                              center * descriptor.sampleRate/*312500.0*/).GetFreqString();
        powerStr.sprintf("%.3f dBm", power);
        relativeStr.sprintf("%.3f dB", diff);
        averageStr.sprintf("%.3f dB", avgPower);

        emit updateLabels(centerStr, powerStr, relativeStr, averageStr);
    }

    device->Abort();
}
