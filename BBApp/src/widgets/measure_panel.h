#ifndef MEASURE_PANEL_H
#define MEASURE_PANEL_H

#include "dock_panel.h"
#include "entry_widgets.h"
#include "lib/bb_lib.h"

class TraceManager;
class SweepSettings;

class MeasurePanel : public DockPanel {
    Q_OBJECT

public:
    MeasurePanel(const QString &title,
                 QWidget *parent,
                 TraceManager *trace_manager,
                 const SweepSettings *sweep_settings);
    ~MeasurePanel();

private:
    DockPage *channel_power_page;
    DockPage *occupied_bandwidth_page;

    // Trace Widgets
    ComboEntry *trace_select;
    ComboEntry *trace_type;
    NumericEntry *trace_avg_count;
    ColorEntry *trace_color;
    CheckBoxEntry *trace_updating;
    DualButtonEntry *export_clear;

    // Marker Widgets
    ComboEntry *marker_select;
    ComboEntry *on_trace_select;
    FrequencyEntry *setMarkerFreq;
    CheckBoxEntry *marker_update;
    CheckBoxEntry *marker_active;
    DualButtonEntry *peak_delta;
    DualButtonEntry *to_center_ref;
    DualButtonEntry *peak_left_right;

    // Offsets
    NumericEntry *ref_offset;

    // Channel Power
    FrequencyEntry *channel_width;
    FrequencyEntry *channel_spacing;
    CheckBoxEntry *channel_power_enabled;

    // Occupied Bandwidth
    CheckBoxEntry *ocbw_enabled;
    NumericEntry *percentPower;

    // Copy of the pointer, does not own
    TraceManager *trace_manager_ptr;
    const SweepSettings *settings_ptr;

public slots:
    void updateTraceView();
    void updateTraceView(int);
    void updateMarkerView();
    void updateMarkerView(int);
    void setMode(OperationalMode mode);

private slots:
    void channelPowerUpdated();
    void occupiedBandwidthUpdated();

    void setMarkerFrequencyChanged(Frequency);

signals:

private:
    DISALLOW_COPY_AND_ASSIGN(MeasurePanel)
};

#endif // MEASURE_PANEL_H
