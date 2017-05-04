#include "trace_manager.h"
#include "sweep_settings.h"
#include "../widgets/entry_widgets.h"

#include <cassert>

#include <QSettings>
#include <QFileDialog>

static QColor default_trace_colors[TRACE_COUNT] = {
    QColor(0, 0, 0),
    QColor(0, 55, 200),
    QColor(255, 0, 0),
    QColor(0, 200, 200),
    QColor(187, 0, 187),
    QColor(150, 150, 150)
};

TraceManager::TraceManager()
{
    LoadColors();

    activeTrace = 0;
    activeMarker = 0;

    // Trace one always on
    //traces[0].Activate(true);
    traces[0].SetType(NORMAL);
    traces[0].SetUpdate(true);

    ref_offset = 0.0;

    lastTraceAboveReference = false;
}

TraceManager::~TraceManager()
{
    SaveColors();
}

void TraceManager::LoadColors()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "SignalHound", "Preferences");

    for(int i = 0; i < TRACE_COUNT; i++) {
        traces[i].SetColor(s.value("TraceColor/Trace" + QVariant(i).toString(),
                                   default_trace_colors[i]).value<QColor>());
    }
}

void TraceManager::SaveColors()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "SignalHound", "Preferences");

    for(int i = 0; i < TRACE_COUNT; i++) {
        s.setValue("TraceColor/Trace" + QVariant(i).toString(), traces[i].Color());
    }
}

void TraceManager::ClearAllTraces()
{
    Lock();

    for(int i = 0; i < TRACE_COUNT; i++) {
        traces[i].SetSize(0);
    }

    Unlock();
}

void TraceManager::Reset()
{
    Lock();

    activeTrace = 0;
    for(int i = 0; i < TRACE_COUNT; i++) {
        //traces[i].Activate(i == 0);
        if(i != 0) traces[i].Disable();
        traces[i].SetUpdate(true);
        traces[i].SetSize(0);
    }

    activeMarker = 0;
    for(int i = 0; i < MARKER_COUNT; i++) {
        markers[i].SetActive(false);
        markers[i].SetUpdate(true);
        markers[i].SetOnTrace(0);
    }

    ref_offset = 0.0;

    pathLoss.Clear();
    limitLine.Clear();

    Unlock();

    emit updated();
}

void TraceManager::UpdateTraces(Trace *trace)
{
    Lock();

    // Apply ref-offset first
    if(ref_offset != 0.0) {
        trace->ApplyOffset(ref_offset);
    }

    pathLoss.Apply(trace);

    // Determine if the maximum value is above the reference level
    double peak_freq, peak_amp;
    Amplitude ref = trace->GetSettings()->RefLevel();
    trace->GetSignalPeak(&peak_freq, &peak_amp);
    if(!ref.IsLogScale()) {
        lastTraceAboveReference = (peak_amp > ref.Val());
    } else {
        // Log Scale
        lastTraceAboveReference = (peak_amp > ref.ConvertToUnits(AmpUnits::DBM));
    }

    // Limit lines should be tested after any amplitude offsets
    limitLine.Apply(trace);

    // Iterate through and update all traces
    for(int i = 0; i < TRACE_COUNT; i++) {
        traces[i].Update(*trace);
    }

    Unlock();

    if(trace->IsFullSweep()) {
        // Place trace in our persist/waterfall buffer
        normalize_trace(trace, *trace_buffer.Front(), QPoint(1280, 720));
        trace_buffer.IncrementFront();
    }

    channel_power.Update(trace);

    if(ocbw.enabled) {
        trace->GetOccupiedBandwidth(ocbw);
    }
}

int TraceManager::SolveMarkers(const SweepSettings *s)
{
    for(int i = 0; i < MARKER_COUNT; i++) {
        GetMarker(i)->UpdateMarker(GetTrace(GetMarker(i)->OnTrace()), s);
    }

    return GetVisibleMarkerCount();
}

int TraceManager::SolveMarkersForPhaseNoise(const SweepSettings *s)
{
    for(int i = 0; i < MARKER_COUNT; i++) {
        GetMarker(i)->UpdateMarkerForPhaseNoise(GetTrace(GetMarker(i)->OnTrace()), s);
    }

    return GetVisibleMarkerCount();
}

//int TraceManager::SolveMarkers(const DemodSettings *ds)
//{
//    for(int i = 0; i < MARKER_COUNT; i++) {
//        GetMarker(i)->UpdateMarker(
//                    GetTrace(GetMarker(i)->OnTrace()), ds);
//    }

//    return GetVisibleMarkerCount();
//}

const Trace* TraceManager::GetTrace(int index)
{
    assert(index >= 0 && index < TRACE_COUNT);

    if(index < 0 || index >= TRACE_COUNT)
        return 0;

    return &traces[index];
}

int TraceManager::GetFirstActiveTrace() const
{
    for(int t = 0; t < TRACE_COUNT; t++) {
        if(traces[t].Active()) {
            return t;
        }
    }

    return 0;
}

Marker* TraceManager::GetMarker(int index)
{
    assert(index >= 0 && index < MARKER_COUNT);

    return &markers[index];
}

int TraceManager::GetVisibleMarkerCount()
{
    int count = 0;

    for(int i = 0; i < MARKER_COUNT; i++) {
        if(GetMarker(i)->Active() && GetMarker(i)->InView()) {
            count++;
        }
    }

    return count;
}

void TraceManager::PlaceMarkerFrequency(Frequency f)
{
    int marker_on_trace = GetActiveMarker()->OnTrace();

    if(!traces[marker_on_trace].Active()) {
        marker_on_trace = GetFirstActiveTrace();
        GetActiveMarker()->SetOnTrace(marker_on_trace);
    }

    GetActiveMarker()->Place(f);
    emit updated();
}

void TraceManager::PlaceMarkerPercent(double percent)
{
    int marker_on_trace = GetActiveMarker()->OnTrace();

    if(!traces[marker_on_trace].Active()) {
        marker_on_trace = GetFirstActiveTrace();
        GetActiveMarker()->SetOnTrace(marker_on_trace);
    }

    const Trace *t = GetTrace(marker_on_trace);
    const SweepSettings *s = traces[marker_on_trace].GetSettings();
    double span = t->BinSize() * t->Length();

    GetActiveMarker()->Place(t->StartFreq() + span * percent, percent);
    //GetActiveMarker()->Place(s->Start() + s->Span() * percent, percent);

    emit updated();
}

//void TraceManager::PlaceMarkerPhaseNoise(double percent)
//{
//    int marker_on_trace = GetActiveMarker()->OnTrace();

//    if(!traces[marker_on_trace].Active()) {
//        marker_on_trace = GetFirstActiveTrace();
//        GetActiveMarker()->SetOnTrace(marker_on_trace);
//    }

//    const SweepSettings *s = traces[marker_on_trace].GetSettings();
//    GetActiveMarker()->Place(s->Start() + s->Span() * percent, percent);

//    emit updated();
//}

void TraceManager::BumpMarker(bool right)
{
    Marker *marker = GetActiveMarker();

    double constant = right ? 1.0 : -1.0;
    marker->AdjustFrequency(traces[marker->OnTrace()].BinSize() * constant, right);

    //marker->AdjustMarker(right);
}

void TraceManager::setActiveIndex(int index)
{
    assert(index >= 0 && index < TRACE_COUNT);

    if(index < 0 || index >= TRACE_COUNT)
        return;

    activeTrace = index;
    emit updated();
}

void TraceManager::setActive(bool active)
{
    //GetActiveTrace()->Activate(active);
    emit updated();
}

void TraceManager::setUpdate(bool update)
{
    GetActiveTrace()->SetUpdate(update);
    emit updated();
}

void TraceManager::setType(int type)
{
    GetActiveTrace()->SetType((TraceType)type);
    emit updated();
}

void TraceManager::setAvgCount(double count)
{
    GetActiveTrace()->SetAvgCount((int)count);
    emit updated();
}

void TraceManager::setColor(QColor &color)
{
    GetActiveTrace()->SetColor(color);
    emit updated();
}

void TraceManager::toFront()
{

}

void TraceManager::clearTrace()
{
    GetActiveTrace()->Clear();
}

void TraceManager::exportTrace()
{
    QString fileName = QFileDialog::getSaveFileName(0,
                                                    tr("Export File Name"),
                                                    sh::GetDefaultExportDirectory(),
                                                    tr("CSV Files (*.csv)"));

    if(fileName.isNull()) return;

    Lock();
    GetActiveTrace()->Export(fileName);
    Unlock();

    sh::SetDefaultExportDirectory(QFileInfo(fileName).absoluteDir().absolutePath());
}

void TraceManager::clearAll()
{

    emit updated();
}

void TraceManager::setActiveMarkerIndex(int index)
{
    assert(index >= 0 && index < MARKER_COUNT);

    if(activeMarker == index) {
        return;
    }

    activeMarker = index;
    emit updated();
}

void TraceManager::setMarkerOnTrace(int trace)
{
    assert(trace >= 0 && trace < TRACE_COUNT);

    if(GetActiveMarker()->OnTrace() == trace) {
        return;
    }

    GetActiveMarker()->SetOnTrace(trace);
    emit updated();
}

void TraceManager::markerPeakSearch()
{
    int t = GetActiveMarker()->OnTrace();

    double freq = 0.0;
    GetTrace(t)->GetSignalPeak(&freq, nullptr);
    if(GetActiveMarker()->Place(freq)) {
        emit updated();
    }

    //int ix = GetTrace(t)->GetPeakIndex();
    //GetActiveMarker()->Place((double)ix / GetTrace(t)->Length());
    //GetActiveMarker()->Place(ix);

    emit updated();
}

void TraceManager::markerDeltaClicked()
{
    GetActiveMarker()->EnableDelta();
    emit updated();
}

void TraceManager::markerToCenter()
{
    if(GetActiveMarker()->Active())
        emit changeCenterFrequency(
                GetActiveMarker()->Freq());
}

void TraceManager::markerToRef()
{
    if(GetActiveMarker()->Active())
        emit changeReferenceLevel(
                GetActiveMarker()->Amp());
}

void TraceManager::markerPeakLeft()
{
    Marker *marker_ptr = GetActiveMarker();
    const Trace *trace_ptr = &traces[marker_ptr->OnTrace()];

    if(!marker_ptr->Active()) {
        markerPeakSearch();
        return;
    }

    std::vector<int> peak_list;
    trace_ptr->GetPeakList(peak_list);

    if(peak_list.size() == 0) {
        return;
    }

    int peak_ix = 0;
    int old_ix = marker_ptr->Index();

    while(peak_ix < peak_list.size()) {
        if(peak_list[peak_ix] >= old_ix) {
            break;
        }
        peak_ix++;
    }

    if(peak_ix == 0) {
        // do nothing
    } else {
        marker_ptr->Place(trace_ptr->StartFreq() +
                          trace_ptr->BinSize() * peak_list[peak_ix-1]);
        //marker_ptr->Place((double)peak_list[peak_ix-1] / trace_ptr->Length());
        //marker_ptr->Place(peak_list[peak_ix-1]);
    }

    emit updated();
}

void TraceManager::markerPeakRight()
{
    Marker *marker_ptr = GetActiveMarker();
    const Trace *trace_ptr = &traces[marker_ptr->OnTrace()];

    if(!marker_ptr->Active()) {
        markerPeakSearch();
        return;
    }

    std::vector<int> peak_list;
    trace_ptr->GetPeakList(peak_list);

    if(peak_list.size() == 0) {
        return;
    }

    int peak_ix = 0;
    int old_ix = marker_ptr->Index();

    while(peak_ix < peak_list.size()) {
        if(peak_list[peak_ix] > old_ix) {
            break;
        }
        peak_ix++;
    }

    //int final_ix = (peak_ix == 0) ? 0 : peak_ix + 1 ;
    //if(peak_list[final_ix] == old_ix) final_ix++;

    if(peak_ix >= peak_list.size()) {
        // Do nothing if at last peak
    } else {
        marker_ptr->Place(trace_ptr->StartFreq() +
                          trace_ptr->BinSize() * peak_list[peak_ix]);
        //marker_ptr->Place((double)peak_list[peak_ix] / trace_ptr->Length());
        //marker_ptr->Place(peak_list[peak_ix]);
    }

    emit updated();
}

void TraceManager::markerUpdate(bool update)
{
    if(update == GetActiveMarker()->Updating()) {
        return;
    }

    GetActiveMarker()->SetUpdate(update);
    emit updated();
}

void TraceManager::markerActive(bool active)
{
    if(active == GetActiveMarker()->Active()) {
        return;
    }

    GetActiveMarker()->SetActive(active);
    emit updated();
}

void TraceManager::setRefOffset(double offset)
{
    ref_offset = offset;
    emit updated();
}

void TraceManager::SetChannelPower(bool enable, Frequency width, Frequency spacing)
{
    channel_power.Configure(enable, width, spacing);
}

void TraceManager::SetOccupiedBandwidth(bool enabled, double percentPower)
{
    ocbw.percentPower = percentPower;
    ocbw.enabled = enabled;
}

void TraceManager::importPathLoss()
{
    QString fileName = QFileDialog::getOpenFileName(0, tr("Select Path-Loss CSV"),
                                                    bb_lib::get_my_documents_path(),
                                                    tr("CSV File (*.csv)"));
    pathLoss.Import(fileName);
}

void TraceManager::clearPathLoss()
{
    pathLoss.Clear();
}

void TraceManager::importLimitLines()
{
    QString fileName = QFileDialog::getOpenFileName(0, tr("Select Path-Loss CSV"),
                                                    bb_lib::get_my_documents_path(),
                                                    tr("CSV File (*.csv)"));
    limitLine.Import(fileName);
}

void TraceManager::clearLimitLines()
{
    limitLine.Clear();
}
