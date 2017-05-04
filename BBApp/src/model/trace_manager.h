#ifndef TRACE_MANAGER_H
#define TRACE_MANAGER_H

#include <array>

#include <QMutex>
#include <QObject>

#include "../lib/macros.h"
#include "../lib/threadsafe_queue.h"
#include "trace.h"
#include "marker.h"
#include "persistence.h"
#include "import_table.h"

class Settings;
class DemodSettings;

/*
 * One copy found in the Session class.
 * Represents 6 traces and 6 markers
 * Has one "active" trace at a time.
 * Most methods forward messages to that trace.
 * This prevents anyone from having non-const
 *   access to any trace except this class.
 * The TracePanel is the only dock widget that modifies
 *   this class.
 */
class TraceManager : public QObject {
    Q_OBJECT

public:
    TraceManager();
    ~TraceManager();

    // Lock and Unlock access to the traces
    void Lock() { modMutex.lock(); }
    void Unlock() { modMutex.unlock(); }

    // Trace Manager saves trace colors to .ini file on open/close
    void LoadColors();
    void SaveColors();

    // Clear traces on new configuration
    void ClearAllTraces();
    // Load default settings, used on device close
    void Reset();

    // Update all traces with trace from device
    void UpdateTraces(Trace *trace);

    // Returns the number of active and in-view markers
    int SolveMarkers(const SweepSettings *s); // For freq sweeps
    int SolveMarkersForPhaseNoise(const SweepSettings *s); // For phase noise sweeps
    //int SolveMarkers(const DemodSettings *ds); // For time sweeps

    // Modify active index
    int GetActiveTraceIndex() const {
        return activeTrace;
    }

    // Get traces
    Trace* GetActiveTrace() { return &traces[activeTrace]; }
    const Trace* GetTrace(int index);
    // Return the index of the first found active trace
    // If no active trace, return 0
    int GetFirstActiveTrace() const;

    int GetActiveMarkerIndex() const { return activeMarker; }
    Marker* GetActiveMarker() { return &markers[activeMarker]; }
    Marker* GetMarker(int index);

    // Get the number of active and inview markers
    // Call after SolveMarkers()
    int GetVisibleMarkerCount();
    // Place the current marker on the current trace
    void PlaceMarkerFrequency(Frequency f); // Place on frequency
    void PlaceMarkerPercent(double percent); // Place with percent
    //void PlaceMarkerPhaseNoise(double percent);
    // Move active marker one step, if(right) increase frequency
    void BumpMarker(bool right);

    const PathLossTable& GetPathLossTable() const { return pathLoss; }
    const LimitLineTable* GetLimitLine() const { return &limitLine; }

    double RefOffset() const { return ref_offset; }

    void SetChannelPower(bool enable, Frequency width, Frequency spacing);
    const ChannelPower* GetChannelPowerInfo() const { return &channel_power; }

    void SetOccupiedBandwidth(bool enabled, double percentPower);
    const OccupiedBandwidthInfo& GetOccupiedBandwidthInfo() const { return ocbw; }

    // Real-Time and Waterfall trace buffer
    ThreadSafeQueue<GLVector, 32> trace_buffer;

    RealTimeFrame realTimeFrame;

    bool LastTraceAboveReference() const { return lastTraceAboveReference; }

protected:

private:
    // 6 traces and current index
    int activeTrace;
    Trace traces[TRACE_COUNT];
    // 6 markers and current index
    int activeMarker;
    Marker markers[MARKER_COUNT];
    // Use to lock access to traces
    QMutex modMutex;

    PathLossTable pathLoss;
    LimitLineTable limitLine;

    double ref_offset; // dB

    ChannelPower channel_power;
    OccupiedBandwidthInfo ocbw;

    bool lastTraceAboveReference;

public slots:
    // Modifies the active trace or sets the active trace
    void setActiveIndex(int);
    void setActive(bool);
    void setUpdate(bool);
    void setType(int);
    void setAvgCount(double);
    void setColor(QColor &);
    void toFront();
    void clearTrace();
    void exportTrace();
    void clearAll(); // Back to default settings

    // Marker functions, modifies the active marker
    //  or sets the active marker
    void setActiveMarkerIndex(int);
    void setMarkerOnTrace(int);
    void markerPeakSearch();
    void markerDeltaClicked();
    void markerToCenter();
    void markerToRef();
    void markerPeakLeft();
    void markerPeakRight();
    void markerUpdate(bool);
    void markerActive(bool);

    void setRefOffset(double);

    void importPathLoss();
    void clearPathLoss();
    void importLimitLines();
    void clearLimitLines();

//    void setChannelPower(bool enable);
//    void setChannelWidth(Frequency width);
//    void setChannelSpacing(Frequency spacing);

signals:
    // Emitted when some variable was set through the setters
    void updated();
    //
    void changeCenterFrequency(Frequency center);
    void changeReferenceLevel(Amplitude amplitude);

private:
    DISALLOW_COPY_AND_ASSIGN(TraceManager)
};

#endif // TRACE_MANAGER_H
