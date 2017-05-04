#include "measure_panel.h"
#include "../model/trace_manager.h"

#include <QMessageBox>

MeasurePanel::MeasurePanel(const QString &title,
                           QWidget *parent,
                           TraceManager *trace_manager,
                           const SweepSettings *sweep_settings) :
    DockPanel(title, parent),
    trace_manager_ptr(trace_manager),
    settings_ptr(sweep_settings)
{
    DockPage *trace_page = new DockPage("Traces");
    DockPage *marker_page = new DockPage("Markers");
    DockPage *offset_page = new DockPage("Offsets");
    channel_power_page = new DockPage("Channel Power");
    occupied_bandwidth_page = new DockPage("Occupied Bandwidth");

    QStringList string_list;

    trace_select = new ComboEntry("Trace");
    trace_type = new ComboEntry("Type");
    trace_avg_count = new NumericEntry("Avg Count", 10, "");
    trace_color = new ColorEntry("Color");
    //trace_active = new CheckBoxEntry("Active");
    trace_updating = new CheckBoxEntry("Update");
    export_clear = new DualButtonEntry("Export", "Clear");

    string_list << "One" << "Two" << "Three" << "Four" << "Five" << "Six";
    trace_select->setComboText(string_list);
    string_list.clear();

    // Must match TraceType enum list
    string_list << "Off" << "Clear & Write" << "Max Hold" << "Min Hold" <<
                   "Min/Max Hold" << "Average";
    trace_type->setComboText(string_list);
    string_list.clear();

    trace_page->AddWidget(trace_select);
    trace_page->AddWidget(trace_type);
    trace_page->AddWidget(trace_avg_count);
    trace_page->AddWidget(trace_color);
    trace_page->AddWidget(trace_updating);
    trace_page->AddWidget(export_clear);

    AppendPage(trace_page);

    connect(trace_select, SIGNAL(comboIndexChanged(int)),
            this, SLOT(updateTraceView(int)));
    connect(trace_type, SIGNAL(comboIndexChanged(int)),
            trace_manager_ptr, SLOT(setType(int)));
    connect(trace_avg_count, SIGNAL(valueChanged(double)),
            trace_manager_ptr, SLOT(setAvgCount(double)));
    connect(trace_color, SIGNAL(colorChanged(QColor&)),
            trace_manager_ptr, SLOT(setColor(QColor&)));
    connect(trace_updating, SIGNAL(clicked(bool)),
            trace_manager_ptr, SLOT(setUpdate(bool)));

    connect(export_clear, SIGNAL(leftPressed()),
            trace_manager_ptr, SLOT(exportTrace()));
    connect(export_clear, SIGNAL(rightPressed()),
            trace_manager_ptr, SLOT(clearTrace()));

    // Marker stuff
    marker_select = new ComboEntry("Marker");
    on_trace_select = new ComboEntry("Place On");
    setMarkerFreq = new FrequencyEntry("Set Freq", 1.0e6);
    marker_update = new CheckBoxEntry("Update");
    marker_active = new CheckBoxEntry("Active");
    peak_delta = new DualButtonEntry("Peak Search", "Delta");
    to_center_ref = new DualButtonEntry("To Center", "To Ref");
    peak_left_right = new DualButtonEntry("Peak Left", "Peak Right");

    string_list << "One" << "Two" << "Three" << "Four" << "Five" << "Six";
    marker_select->setComboText(string_list);
    string_list.clear();
    string_list << "Trace One" << "Trace Two" << "Trace Three"
                << "Trace Four" << "Trace Five" << "Trace Six";
    on_trace_select->setComboText(string_list);
    string_list.clear();

    marker_page->AddWidget(marker_select);
    marker_page->AddWidget(on_trace_select);
    marker_page->AddWidget(setMarkerFreq);
    marker_page->AddWidget(marker_update);
    marker_page->AddWidget(marker_active);
    marker_page->AddWidget(peak_delta);
    marker_page->AddWidget(to_center_ref);
    marker_page->AddWidget(peak_left_right);

    AppendPage(marker_page);

    connect(marker_select, SIGNAL(comboIndexChanged(int)),
            this, SLOT(updateMarkerView(int)));
    connect(on_trace_select, SIGNAL(comboIndexChanged(int)),
            trace_manager_ptr, SLOT(setMarkerOnTrace(int)));
    connect(setMarkerFreq, SIGNAL(freqViewChanged(Frequency)),
            this, SLOT(setMarkerFrequencyChanged(Frequency)));
    connect(marker_update, SIGNAL(clicked(bool)),
            trace_manager_ptr, SLOT(markerUpdate(bool)));
    connect(marker_active, SIGNAL(clicked(bool)),
            trace_manager_ptr, SLOT(markerActive(bool)));
    connect(peak_delta, SIGNAL(leftPressed()),
            trace_manager_ptr, SLOT(markerPeakSearch()));
    connect(peak_delta, SIGNAL(rightPressed()),
            trace_manager_ptr, SLOT(markerDeltaClicked()));
    connect(to_center_ref, SIGNAL(leftPressed()),
            trace_manager_ptr, SLOT(markerToCenter()));
    connect(to_center_ref, SIGNAL(rightPressed()),
            trace_manager_ptr, SLOT(markerToRef()));
    connect(peak_left_right, SIGNAL(leftPressed()),
            trace_manager_ptr, SLOT(markerPeakLeft()));
    connect(peak_left_right, SIGNAL(rightPressed()),
            trace_manager_ptr, SLOT(markerPeakRight()));

    ref_offset = new NumericEntry("Ref Offset",
                                  trace_manager_ptr->RefOffset(),
                                  "dB");

    offset_page->AddWidget(ref_offset);

    AppendPage(offset_page);

    connect(ref_offset, SIGNAL(valueChanged(double)),
            trace_manager_ptr, SLOT(setRefOffset(double)));

    channel_width = new FrequencyEntry("Width",
                                       20.0e6);
    channel_spacing = new FrequencyEntry("Spacing", 20.0e6);
    channel_power_enabled = new CheckBoxEntry("Enabled");

    channel_power_page->AddWidget(channel_width);
    channel_power_page->AddWidget(channel_spacing);
    channel_power_page->AddWidget(channel_power_enabled);

    AppendPage(channel_power_page);

    connect(channel_width, SIGNAL(freqViewChanged(Frequency)),
            this, SLOT(channelPowerUpdated()));
    connect(channel_spacing, SIGNAL(freqViewChanged(Frequency)),
            this, SLOT(channelPowerUpdated()));
    connect(channel_power_enabled, SIGNAL(clicked(bool)),
            this, SLOT(channelPowerUpdated()));

    ocbw_enabled = new CheckBoxEntry("Enabled");
    percentPower = new NumericEntry("% Power", 0.0, "");
    percentPower->SetValue(99.0);

    occupied_bandwidth_page->AddWidget(ocbw_enabled);
    occupied_bandwidth_page->AddWidget(percentPower);

    AppendPage(occupied_bandwidth_page);

    connect(ocbw_enabled, SIGNAL(clicked(bool)), SLOT(occupiedBandwidthUpdated()));
    connect(percentPower, SIGNAL(valueChanged(double)), SLOT(occupiedBandwidthUpdated()));

    // Done connected DockPages to TraceManager
    updateTraceView(0);
    updateMarkerView(0);

    // Miscellaneous connects
    connect(trace_manager_ptr, SIGNAL(updated()),
            this, SLOT(updateTraceView()));
    connect(trace_manager_ptr, SIGNAL(updated()),
            this, SLOT(updateMarkerView()));
}

MeasurePanel::~MeasurePanel()
{
    // Don't delete widgets on pages
}

/*
 * Change everything, triggered from change in trace manager
 */
void MeasurePanel::updateTraceView()
{
    int active_ix = trace_manager_ptr->GetActiveTraceIndex();
    const Trace *t = trace_manager_ptr->GetActiveTrace();
    TraceType type = (TraceType)t->GetType();

    trace_select->setComboIndex(active_ix);
    trace_type->setComboIndex(type);
    //trace_avg_count->setEnabled(type == AVERAGE);
    trace_avg_count->SetValue(t->GetAvgCount());
    trace_color->SetColor(t->Color());
    trace_updating->SetChecked(t->IsUpdating());

    ref_offset->SetValue(trace_manager_ptr->RefOffset());
}

// Same as above, but specify index
void MeasurePanel::updateTraceView(int new_ix)
{
    trace_manager_ptr->setActiveIndex(new_ix);
    const Trace *t = trace_manager_ptr->GetActiveTrace();

    trace_select->setComboIndex(new_ix);
    trace_type->setComboIndex((int)t->GetType());
    trace_color->SetColor(t->Color());
    //trace_active->SetChecked(t->Active());
    trace_updating->SetChecked(t->IsUpdating());
}

/*
 * Change manager, triggered from change in trace manager
 */
void MeasurePanel::updateMarkerView()
{
    int active_ix = trace_manager_ptr->GetActiveMarkerIndex();
    const Marker *m = trace_manager_ptr->GetActiveMarker();

    marker_select->setComboIndex(active_ix);
    on_trace_select->setComboIndex(m->OnTrace());
    marker_update->SetChecked(m->Updating());
    marker_active->SetChecked(m->Active());
}

// Same as above, but specify index
void MeasurePanel::updateMarkerView(int new_ix)
{
    trace_manager_ptr->setActiveMarkerIndex(new_ix);
    const Marker *m = trace_manager_ptr->GetActiveMarker();

    marker_select->setComboIndex(new_ix);
    on_trace_select->setComboIndex(m->OnTrace());
    marker_update->SetChecked(m->Updating());
    marker_active->SetChecked(m->Active());
}

void MeasurePanel::setMode(OperationalMode mode)
{
    bool pagesEnabled = !(mode == MODE_NETWORK_ANALYZER);

    channel_power_page->SetPageEnabled(pagesEnabled);
    occupied_bandwidth_page->SetPageEnabled(pagesEnabled);
}

void MeasurePanel::channelPowerUpdated()
{
    if(!settings_ptr->IsAveragePower() && channel_power_enabled->IsChecked()) {
        QMessageBox::warning(0, "Channel Power Warning",
                             "For Accurate Measurements, please set\n"
                             "Detector = Average\n"
                             "Video Units = Power");
    }
    trace_manager_ptr->SetChannelPower(channel_power_enabled->IsChecked(),
                                       channel_width->GetFrequency(),
                                       channel_spacing->GetFrequency());
}

void MeasurePanel::occupiedBandwidthUpdated()
{
    double powerVal = percentPower->GetValue();
    if(powerVal < MIN_OCBW_PERCENT_POWER) percentPower->SetValue(MIN_OCBW_PERCENT_POWER);
    if(powerVal > MAX_OCBW_PERCENT_POWER) percentPower->SetValue(MAX_OCBW_PERCENT_POWER);

    if(!settings_ptr->IsAveragePower() && ocbw_enabled->IsChecked()) {
        QMessageBox::warning(0, "Occupied Bandwidth Warning",
                             "For Accurate Measurements, please set\n"
                             "Detector = Average\n"
                             "Video Units = Power");
    }
    trace_manager_ptr->SetOccupiedBandwidth(ocbw_enabled->IsChecked(),
                                            percentPower->GetValue());
}

void MeasurePanel::setMarkerFrequencyChanged(Frequency f)
{
    if(f.Val() < 0.0) {
        f = 0.0;
        setMarkerFreq->SetFrequency(f);
        return;
    }

    trace_manager_ptr->PlaceMarkerFrequency(f);
}
