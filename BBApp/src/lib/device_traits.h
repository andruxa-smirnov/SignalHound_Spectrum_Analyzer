#ifndef DEVICE_TRAITS_H
#define DEVICE_TRAITS_H

#include <utility>

class SweepSettings;

enum DeviceType {
    DeviceTypeSA44A = 0,
    DeviceTypeSA44B,
    DeviceTypeSA124,
    DeviceTypeBB60A,
    DeviceTypeBB60C
};

// BB60C values by default ?
class device_traits {
public:
    static void set_device_type(DeviceType new_type);

    static double min_span();
    static double min_frequency();
    static double min_iq_frequency();
    static double best_start_frequency();
    static double max_frequency();
    static std::pair<double, double> full_span_frequencies();
    static double adjust_rbw_on_span(const SweepSettings *ss);
    static double adjust_vbw(const SweepSettings *ss);
    static double get_best_rbw(const SweepSettings *ss);
    static double sequence_bw(double bw, bool native_bw, bool increase);
    static double min_real_time_rbw();
    static double max_real_time_rbw();
    static double min_real_time_span();
    static double max_real_time_span();
    static double min_iq_bandwidth();
    static double max_iq_bandwidth(int decimation_order);
    static int default_decimation();
    static int max_atten();
    static int max_gain();
    static double sample_rate();
    static bool default_spur_reject();
    static int mod_analysis_decimation_order();
    static int audio_rate();
    static double get_flattop_window_bandwidth(double rbw);
    static bool has_native_bandwidths();

private:
    static DeviceType type;
};

#endif // DEVICE_TRAITS_H
