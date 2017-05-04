#ifndef SWEEP_PANEL_H
#define SWEEP_PANEL_H

#include "dock_panel.h"
#include "entry_widgets.h"
#include "lib/bb_lib.h"

class SweepSettings;
class Device;
class Session;

class SweepPanel : public DockPanel {
    Q_OBJECT

    Session *sessionPtr; // Does not own

public:
    SweepPanel(const QString &title,
               QWidget *parent,
               Session *session);
    ~SweepPanel();

    void DeviceConnected(DeviceType type);

private:
    DockPage *tg_page;
    DockPage *frequency_page, *amplitude_page;
    DockPage *bandwidth_page, *acquisition_page;

    NumericEntry *tgSweepSize;
    CheckBoxEntry *tgHighDynamicRange;
    ComboEntry *tgSweepType;
    DualButtonEntry *tgStoreThru;

    FreqShiftEntry *center;
    FreqShiftEntry *span;
    FrequencyEntry *start;
    FrequencyEntry *stop;
    FrequencyEntry *step;
    DualButtonEntry *full_zero_span;

    AmplitudeEntry *ref;
    NumericEntry *div;
    ComboEntry *gain;
    ComboEntry *atten;
    ComboEntry *preamp;

    CheckBoxEntry *native_rbw;
    FreqShiftEntry *rbw;
    FreqShiftEntry *vbw;
    CheckBoxEntry *auto_rbw;
    CheckBoxEntry *auto_vbw;

    ComboEntry *video_units;
    ComboEntry *detector;
    TimeEntry *sweep_time;

public slots:
    void updatePanel(const SweepSettings *settings);
    void setMode(OperationalMode mode);
    void enableManualGainAtten(bool enable) {
        gain->setEnabled(enable);
        atten->setEnabled(enable);
        preamp->setEnabled(enable);
    }

private slots:
    void storeThrough();
    void storeThroughPad();

signals:
    void zeroSpanPressed();

private:
    DISALLOW_COPY_AND_ASSIGN(SweepPanel)
};

#endif // SWEEP_PANEL_H
