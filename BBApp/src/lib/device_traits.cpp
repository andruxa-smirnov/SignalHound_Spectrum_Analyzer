#include "device_traits.h"

#include "bb_lib.h"
#include "sa_api.h"
#include "bb_api.h"
#include "model/sweep_settings.h"

#include <QtGlobal>

// I/Q max bandwidth per decimation BB60C
static double max_bw_table_bb60a[] = {
    20.0e6,
    17.8e6,
    8.0e6,
    3.75e6,
    2.0e6,
    1.0e6,
    0.5e6,
    0.25e6
};

// I/Q max bandwidth per decimation BB60C
static double max_bw_table_bb60c[] = {
    27.0e6,
    17.8e6,
    8.0e6,
    3.75e6,
    2.0e6,
    1.0e6,
    0.5e6,
    0.25e6
};

// I/Q max bandwidth per decimation
static double max_bw_table_sa[] = {
    250.0e3,
    225.0e3,
    100.0e3,
    50.0e3,
    20.0e3,
    12.0e3,
    5.0e3,
    3.0e3
};

DeviceType device_traits::type = DeviceTypeBB60C;

void device_traits::set_device_type(DeviceType new_type)
{
    type = new_type;
}

double device_traits::min_span()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 10.0;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return BB_MIN_SPAN;
    }
    return 100.0;
}

double device_traits::min_frequency()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B:
        return SA44_MIN_FREQ;
    case DeviceTypeSA124:
        return SA124_MIN_FREQ;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return BB60_MIN_FREQ;
    }
    return BB60_MIN_FREQ;
}

double device_traits::min_iq_frequency()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B:
        return SA44_MIN_FREQ;
    case DeviceTypeSA124:
        return SA124_MIN_FREQ;
    case DeviceTypeBB60A:
        return 10.0e6;
    case DeviceTypeBB60C:
        return 20.0e6;
    }
    return 20.0e6;
}

double device_traits::best_start_frequency()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 100.0e3;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return 11.0e6;
    }
    return 11.0e6;
}

double device_traits::max_frequency()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B:
        return SA44_MAX_FREQ;
    case DeviceTypeSA124:
        return SA124_MAX_FREQ;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return 6.0e9;
    }
    return 6.0e9;
}

std::pair<double, double> device_traits::full_span_frequencies()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B:
        return std::make_pair(1.0, SA44_MAX_FREQ);
    case DeviceTypeSA124:
        return std::make_pair(100.0e3, 12.4e9);
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return std::make_pair(9.0e3, 6.0e9);
    }
    return std::make_pair(9.0e3, 6.0e9);
}

double device_traits::adjust_rbw_on_span(const SweepSettings *ss)
{
    switch(type) {
    case DeviceTypeSA44A:
        return bb_lib::sa44a_adjust_rbw_on_span(ss);
    case DeviceTypeSA44B:
        return bb_lib::sa44b_adjust_rbw_on_span(ss);
    case DeviceTypeSA124:
        return bb_lib::sa124_adjust_rbw_on_span(ss);
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return bb_lib::adjust_rbw_on_span(ss);
    }
}

// Clamp VBW based on the sweep engine or as
//  a ratio to RBW
double device_traits::adjust_vbw(const SweepSettings *ss)
{
    double newVBW = ss->VBW();

    switch(type) {
    case DeviceTypeSA44A :
        // No mid-range engine in SA44A
        if(ss->Span() > 200.0e3) {
            if(ss->VBW() < 6.5e3) newVBW = 6.5e3;
        }
        if(ss->RBW() > ss->VBW() * 100.0) {
            newVBW = ss->RBW() / 100.0;
        }
        break;
    case DeviceTypeSA44B: case DeviceTypeSA124:
        if(ss->Span() > 98.0e6 || (ss->Start() < 16.0e6 && ss->Span() > 200.0e3)) {
            if(ss->VBW() < 6.5e3) {
                newVBW = 6.5e3;
            }
        }

        // Mid-range sweep, limit VBW to a specific sweep time
        // All mid-range sweeps, limit to 30Hz
        if(ss->Span() > 200.0e3 && ss->Span() < 98.0e6 && ss->Start() > 16.0e6) {
            double limitVBW = ss->Span() / 80.0e3;
            if(newVBW < limitVBW) newVBW = limitVBW;
            if(newVBW < 30.0) newVBW = 30.0;
        }

//        if(ss->RBW() > ss->VBW() * 100.0) {
//            newVBW = ss->RBW() / 100.0;
//        }
        break;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        if(ss->RBW() > ss->VBW() * 1000.0) {
            newVBW = ss->RBW() / 1000.0;
        }
        break;
    }

    return newVBW;
}

double device_traits::get_best_rbw(const SweepSettings *ss)
{
    switch(type) {
    case DeviceTypeSA44A:
        return bb_lib::sa44a_get_best_rbw(ss);
    case DeviceTypeSA44B: case DeviceTypeSA124:
        return bb_lib::sa_get_best_rbw(ss);
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return bb_lib::get_best_rbw(ss);
    }
}

double device_traits::sequence_bw(double bw, bool native_bw, bool increase)
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B:
        return bb_lib::sa44_sequence_bw(bw, native_bw, increase);
    case DeviceTypeSA124:
        return bb_lib::sa124_sequence_bw(bw, native_bw, increase);
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return bb_lib::bb_sequence_bw(bw, native_bw, increase);
    }
}

double device_traits::min_real_time_rbw()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return SA_MIN_RT_RBW;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return BB_MIN_RT_RBW;
    }
    return BB_MIN_RT_RBW;
}

double device_traits::max_real_time_rbw()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return SA_MAX_RT_RBW;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return BB_MAX_RT_RBW;
    }
    return BB_MAX_RT_RBW;
}

double device_traits::min_real_time_span()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 1.0e3;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return BB_MIN_RT_SPAN;
    }
    return BB_MIN_RT_SPAN;
}

double device_traits::max_real_time_span()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 250.0e3;
    case DeviceTypeBB60A:
        return BB60A_MAX_RT_SPAN;
    case DeviceTypeBB60C:
        return BB60C_MAX_RT_SPAN;
    }
    return BB_MIN_RT_SPAN;
}

double device_traits::min_iq_bandwidth()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 1.0e3;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return 100.0e3;
    }
    return 100.0e3;
}

double device_traits::max_iq_bandwidth(int decimation_order)
{
    Q_ASSERT(decimation_order >= 0 && decimation_order <= 7);

    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return max_bw_table_sa[decimation_order];
    case DeviceTypeBB60A:
        return max_bw_table_bb60a[decimation_order];
    case DeviceTypeBB60C:
        return max_bw_table_bb60c[decimation_order];
    }
    return max_bw_table_bb60c[decimation_order];
}

int device_traits::default_decimation()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 0;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return 6;
    }
    return 6;
}

int device_traits::max_atten()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 2;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return 3;
    }
    return 3;
}

int device_traits::max_gain()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 2;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return 3;
    }
    return 3;
}

double device_traits::sample_rate()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 486.111e3;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return 40.0e6;
    }
    return 40.0e6;
}

bool device_traits::default_spur_reject()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return true;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return false;
    }
    return false;
}

int device_traits::mod_analysis_decimation_order()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 0;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return 7;
    }
    return 7;
}

int device_traits::audio_rate()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return 30382;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return 32000;
    }
    return 32000;
}

inline int fft_size_from_flattop_rbw_sa(const double rbw)
{
    double min_bin_sz = rbw / 3.2;
    double min_fft = 486111.111 / min_bin_sz;
    int order = (int)ceil(bb_lib::log2(min_fft));
    return bb_lib::pow2(order);
}

inline int fft_size_from_flattop_rbw_bb(const double rbw)
{
    double min_bin_sz = rbw / 3.2;
    double min_fft = 80.0e6 / min_bin_sz;
    int order = (int)ceil(bb_lib::log2(min_fft));
    return bb_lib::pow2(order);
}

inline double get_flattop_bin_sa(double rbw)
{
    return (rbw * (double)fft_size_from_flattop_rbw_sa(rbw)) / 486111.111;
}

inline double get_flattop_bin_bb(double rbw)
{
    return (rbw * (double)fft_size_from_flattop_rbw_bb(rbw)) / 80.0e6;
}

double device_traits::get_flattop_window_bandwidth(double rbw)
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return get_flattop_bin_sa(rbw);
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return get_flattop_bin_bb(rbw);
    }
}

bool device_traits::has_native_bandwidths()
{
    switch(type) {
    case DeviceTypeSA44A: case DeviceTypeSA44B: case DeviceTypeSA124:
        return false;
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return true;
    }
}
