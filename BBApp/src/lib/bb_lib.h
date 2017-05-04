#ifndef BB_LIB_H
#define BB_LIB_H

#include <cmath>
#include <cassert>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

#include <malloc.h>

#include <QDateTime>
#include <QWaitCondition>
#include <QDebug>
#include <QOpenGLFunctions>
#include <QColor>

#include "frequency.h"
#include "amplitude.h"
#include "time_type.h"
#include "kiss_fft/kissfft.hh"

#include "lib/device_traits.h"

class Trace;

#define INDEX_OFFSET(x) ((GLvoid*)x)

typedef unsigned long ulong;
struct complex_f { float re, im; };

typedef QVector<QPointF> LineList;
typedef std::vector<float> GLVector;

struct bandwidth_lut { double bw, fft_size; };
extern bandwidth_lut native_bw_lut[];

struct span_to_bandwidth { double span, nbw, nnbw; };
extern span_to_bandwidth auto_bw_lut[];
struct sa_span_to_bandwidth { double span, rbw; };
extern sa_span_to_bandwidth sa_auto_bw_lut[];

// Must correspond to combo-box indices
enum WaterfallState { WaterfallOFF = 0, Waterfall2D = 1, Waterfall3D = 2 };
enum BBStyleSheet { LIGHT_STYLE_SHEET = 0, DARK_STYLE_SHEET = 1, BLUE_STYLE_SHEET = 2 };

// GLSL shaders
extern char *persist_vs, *persist_fs;
extern char *dot_persist_temp_vs, *dot_persist_temp_fs;

// Software Mode/State, one value stored in Settings
enum OperationalMode {
    MODE_IDLE = -1,
    MODE_SWEEPING = 0,
    MODE_REAL_TIME = 1,
    MODE_ZERO_SPAN = 2,
    MODE_HARMONICS = 3,
    MODE_NETWORK_ANALYZER = 4,
    MODE_PHASE_NOISE = 5
};

enum Setting {
    CENTER = 0,
    SPAN = 1,
    START = 2,
    STOP = 3,
    RBW = 4,
    VBW = 5
};

enum GLShaderType {
    VERTEX_SHADER,
    FRAGMENT_SHADER
};

class GLShader {
public:
    GLShader(GLShaderType type, char *file_name);
    ~GLShader();

    // Must be called with an active context
    bool Compile(QOpenGLFunctions *gl);

    bool Compiled() const { return (compiled == GL_TRUE); }
    GLuint Handle() const { return shader_handle; }

private:
    GLShaderType shader_type;
    GLuint shader_handle;
    char *shader_source;
    int compiled;
};

class GLProgram {
public:
    GLProgram(char *vertex_file,
              char *fragment_file);
    ~GLProgram();

    bool Compile(QOpenGLFunctions *gl);
    bool Compiled() const { return (compiled == GL_TRUE); }
    GLuint ProgramHandle() const { return program_handle; }

    GLShader* GetShader(GLShaderType type) {
        if(type == VERTEX_SHADER)
            return &vertex_shader;
        else
            return &fragment_shader;
    }

private:
    GLuint program_handle;
    GLShader vertex_shader, fragment_shader;
    int compiled;
};

class SleepEvent {
public:
    SleepEvent() { mut.lock(); }
    ~SleepEvent() { Wake(); mut.unlock(); }

    void Sleep(ulong ms = 0xFFFFFFFFUL) { wait_con.wait(&mut, ms); }
    void Wake() { wait_con.wakeAll(); }

private:
    QMutex mut;
    QWaitCondition wait_con;
};

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

    // Windows Event semaphore, At one point I tested this
    //   to be more efficient than a condition variable approach
    class semaphore {
        HANDLE handle;
    public:
        semaphore() { handle = CreateEventA(nullptr, false, false, nullptr); }
        ~semaphore() { CloseHandle(handle); }

        void wait() { WaitForSingleObject(handle, INFINITE); }
        void notify() { SetEvent(handle); }
    };

#else // Linux

    // Semaphore using C++ stdlib
    class semaphore {
        std::mutex m;
        std::condition_variable cv;
        bool set;

    public:
        semaphore() : set(false) {}
        ~semaphore() { cv.notify_all(); }

        void wait() {
            std::unique_lock<std::mutex> lg(m);
            while(!set) { // while() handles spurious wakeup
                cv.wait(lg);//, [=]{ return set; });
            }
            set = false;
        }

        void notify() {
            std::lock_guard<std::mutex> lg(m);
            if(!set) {
                set = true;
                cv.notify_all();
            }
        }
    };

#endif // Semaphore

GLuint get_texture_from_file(const QString &file_name);

inline void glQColor(QColor c)
{
    glColor3f(c.redF(), c.greenF(), c.blueF());
}

inline void glQClearColor(QColor c, float alpha = 0.0)
{
    glClearColor(c.redF(), c.greenF(), c.blueF(), alpha);
}

inline void normalize(float *f) // float f[3]
{
    float invMag , mag;
    mag = sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
    if(mag == 0) mag = 0.1e-5f;
    invMag = (float)1.0 / mag;
    f[0] *= invMag;
    f[1] *= invMag;
    f[2] *= invMag;
}

inline void cross_product(float* r , float* a , float* b)
{
    r[0] = a[1]*b[2] - a[2]*b[1];
    r[1] = a[2]*b[0] - a[0]*b[2];
    r[2] = a[0]*b[1] - a[1]*b[0];
}

// 2x2 matrix determinant
inline float determinant(float a, float b, float c, float d)
{
    return a*d - b*c;
}

inline void swap(float* f1, float* f2)
{
    float temp = *f1;
    *f1 = *f2;
    *f2 = temp;
}

inline void sphereToCart(float theta, float phi, float rho,
                         float *x, float *y, float *z)
{
    *x = rho*sin(phi)*cos(theta);
    *y = rho*sin(phi)*sin(theta);
    *z = rho*cos(phi);
}

inline float* simdMalloc_32f(int len)
{
    return (float*)_aligned_malloc(len * sizeof(float), 32);
}

inline complex_f* simdMalloc_32fc(int len)
{
    return (complex_f*)_aligned_malloc(len * sizeof(complex_f), 32);
}

inline void simdFree(void *ptr)
{
    if(!ptr) return;
    _aligned_free(ptr);
}

inline void simdCopy_32f(const float *src, float *dst, int len)
{
    memcpy(dst, src, len * sizeof(float));
}

inline void simdCopy_32fc(const complex_f *src, complex_f *dst, int len)
{
    memcpy(dst, src, len * sizeof(complex_f));
}

inline void simdMove_32f(const float *src, float *dst, int len)
{
    memmove(dst, src, len * sizeof(float));
}

inline void simdMove_32fc(const complex_f *src, complex_f *dst, int len)
{
    memmove(dst, src, len * sizeof(complex_f));
}

inline void simdZero_32s(int *srcDst, int len)
{
    memset(srcDst, 0, len * sizeof(int));
}

// In-place possible
inline void simdMul_32fc(const complex_f *src1, const complex_f *src2, complex_f *dst, int len)
{
    for(int i = 0 ; i < len; i++) {
        float r = src1[i].re * src2[i].re - src1[i].im * src2[i].im;
        dst[i].im = src1[i].re * src2[i].im + src1[i].im * src2[i].re;
        dst[i].re = r;
    }
}

//template<class FloatType>
//inline FloatType averagePower(const std::vector<FloatType> &input)
//{
//    double sum = 0.0;
//    for(std::vector<FloatType>::size_type i = 0; i < input.size(); i++) {
//        sum += (input[i] * input[i]);
//    }
//    return sum / input.size();
//}

template<class FloatType>
inline FloatType averagePower(const FloatType *input, int len)
{
    assert(len > 0);

    double sum = 0.0;
    for(int i = 0; i < len; i++) {
        sum += (input[i]*input[i]);
    }
    return sum / len;
}

// Correlates the signal in a 10 Hz bandwidth around center in
//   0.2Hz steps. Returns the center and power where the correlation
//   was the strongest.
void getPeakCorrelation(const complex_f *src,
                        int len,
                        double centerIn,
                        double &centerOut,
                        double &peakPower,
                        double sampleRate);

double getSignalFrequency(const std::vector<complex_f> &src, double sampleRate);
QString getSampleRateString(double sampleRate);

// Turn a second value into a string with up to 6 digits of precision
//  using the unit that keeps digits before the decimal to a max of 3
QString getPreciseTimeString(double seconds);

namespace bb_lib {

// Returns true if a new value was retrieved
//bool GetUserInput(const Frequency &fin, Frequency &fout);
//bool GetUserInput(const Amplitude &ain, Amplitude &aout);
//bool GetUserInput(const Time &tin, Time &tout);
//bool GetUserInput(double din, double &dout);

// For copying unicode strings
// Returns length copied, including null char
// maxCopy : maximum size of dst, will copy up to maxCopy-1
//   values, then will null terminate
// Always null terminates
int cpy_16u(const ushort *src, ushort *dst, int maxCopy);

template<class _Type>
inline _Type max2(_Type a, _Type b) {
    return ((a > b) ? a : b);
}

template<class _Type>
inline _Type min2(_Type a, _Type b) {
    return ((a < b) ? a : b);
}

template<class _Type>
inline _Type max3(_Type a, _Type b, _Type c) {
    _Type d = ((a > b) ? a : b);
    return ((d > c) ? d : c);
}

template<class _Type>
inline _Type min3(_Type a, _Type b, _Type c) {
    _Type d = ((a < b) ? a : b);
    return ((d < c) ? d : c);
}

// Clamp val into range of [min, max]
// If val is outside the range specified, clamp val to the
//   range and return true, otherwise return false and do not
//   modify val
template<class Type>
inline bool clamp(Type &val, Type min, Type max) {
    if(val < min) { val = min; return true; }
    if(val > max) { val = max; return true; }
    return false;
}

// Interpolation between a -> b
// p is a 0.0 -> 1.0 value
template<typename _Type>
inline _Type lerp(_Type a, _Type b, float p) {
    return a * (1.f - p) + b * p;
}

// dB to linear voltage correction
// Used for path-loss corrections
inline void db_to_lin(float *srcDst, int len) {
    for(int i = 0; i < len; i++) {
        srcDst[i] = pow(10, srcDst[i] * 0.05);
    }
}

// Convert dBm value to mV
inline void dbm_to_mv(float *srcDst, int len) {
    for(int i = 0; i < len; i++) {
        srcDst[i] = pow(10,(srcDst[i] + 46.9897) * 0.05);
    }
}

// a ^ n
inline double power(double a, int n)
{
    assert(n >= 0);

    if(n == 0) {
        return 1.0;
    }

    int r = a;
    for(int i = 0; i < n-1; i++) {
        r *= a;
    }
    return r;
}

// Get next multiple of 'factor' after start
// e.g. factor = 25, start = 38, return 50
// if start is a multiple of factor, return start
inline double next_multiple_of(double factor, double start)
{
    if(fmod(start, factor) == 0.0)
        return start;

    int m = int(start / factor) + 1;
    return m * factor;
}

// start will be rounded up to the next
inline int next_multiple_of(int factor, int start)
{
    int r = start % factor;

    if(r > 0) {
        return start + (factor - r);
    }

    return start;
}

// Return value [0.0, 1.0], represent fraction of
//   f between [start, stop]
inline double frac_between(double start, double stop, double f) {
    return (f - start) / (stop - start);
}

// Get the closest index representative of the bw parameter
int get_native_bw_index(double bw);
// Get next bandwidth in sequence
double sa44_sequence_bw(double bw, bool native_bw, bool increase);
double sa124_sequence_bw(double bw, bool native_bw, bool increase);
double bb_sequence_bw(double bw, bool native_bw, bool increase);
// Get next 1/2/5
double sequence_span(double span, bool increase);

inline double log2(double val) {
    return log10(val) / log10(2);
}

inline int pow2(int val) {
    if(val < 0) return 0;
    return 0x1 << val;
}

//inline int fft_size_from_non_native_rbw(const double rbw) {
//    double min_bin_sz = rbw / 3.2;
//    double min_fft = 80.0e6 / min_bin_sz;
//    int order = (int)ceil(log2(min_fft));
//    return pow2(order);
//}

// Rounds value up to the closest power of 2
inline unsigned int round_up_power_two(unsigned int val)
{
    int retval = 1;
    val -= 1; // so e.g. 256 becomes 255

    while(val >= 1) {
        val = val >> 1;
        retval = retval << 1;
    }

    return retval;
}

inline unsigned int round_down_power_two(unsigned int val)
{
    int next = round_up_power_two(val);

    if(next == val) {
        return val;
    }

    return next >> 1;
}

//// For non-native bandwidths only
//inline double get_flattop_bandwidth(double rbw) {
//    return (rbw * (double)fft_size_from_non_native_rbw(rbw));
//}

// Adjust rbw to prevent small(<1) and large(>1.2m) sweep sizes
// Return true if adjustment made
double adjust_rbw_on_span(const SweepSettings *ss); // BB60A/C
double sa44a_adjust_rbw_on_span(const SweepSettings *ss);
double sa44b_adjust_rbw_on_span(const SweepSettings *ss);
double sa124_adjust_rbw_on_span(const SweepSettings *ss);

// Retrieve the 'best' possible rbw based on span
// 'best' is determined by the auto_bw_lut[]
double get_best_rbw(const SweepSettings *ss);
double sa_get_best_rbw(const SweepSettings *ss);
double sa44a_get_best_rbw(const SweepSettings *ss);

QString get_my_documents_path();

// Get user selected directory through dialog
// path param is initial directory
QString getUserDirectory(const QString &path);

// n/a for now, shaders are static text strings
char* get_gl_shader_source(const char *file_name);

inline qint64 get_ms_since_epoch() {
    return QDateTime::currentMSecsSinceEpoch();
}
inline QDateTime get_date_time(qint64 ms_since_epoch) {
    return QDateTime::fromMSecsSinceEpoch(ms_since_epoch);
}

// File name for sweep recordings, no milliseconds
inline QString get_recording_filename()
{
    return QDateTime::currentDateTime().toString(
                "yyyy-MM-dd hh'h'mm'm'ss's'") + ".bbr";
}

inline QString get_iq_filename()
{
    return QDateTime::currentDateTime().toString(
                "MM-dd-yy hh'h'mm'm'ss's'");
}

// Text string for widget display purposes, with milliseconds
inline QString get_time_string(int64_t ms_since_epoch) {
    return QDateTime::fromMSecsSinceEpoch(ms_since_epoch).toString(
                "dd/MM/yyyy hh:mm:ss:zzz");
}

} // namespace bb_lib

namespace sh {

const QString GetDefaultImageDirectory();
void SetDefaultImageDirectory(const QString &dir);
const QString GetDefaultExportDirectory();
void SetDefaultExportDirectory(const QString &dir);

bool isOpenGLCompatible();

} // namespace sh

void normalize_trace(const Trace *t, GLVector &vector, QPoint grat_size);
void normalize_trace(const Trace *t, GLVector &vector, QPoint grat_size,
                     Amplitude refLevel, double div);
void normalize_trace(const float *sweepMin, const float *sweepMax,
                     int length, GLVector &v, QPoint grat_size,
                     Amplitude refLevel, double dBdiv);

//void normalize_trace(const Trace *t, LineList &ll, QSize grat_size);

void build_blackman_window(float *dst, int len);
void build_blackman_window(complex_f *dst, int len);
void build_flattop_window(float *dst, int len);
void build_flattop_window(complex_f *dst, int len);

//void demod_am(const complex_f *src, float *dst, int len);
//void demod_fm(const complex_f *src, float *dst, int len, double *phase);
//void demod_fm(const std::vector<complex_f> &src,
//              std::vector<float> &dst, double sampleRate);

// Get index of a trigger
// Returns -1 if not found
// Returns the index after the offending amplitude
int find_rising_trigger(const complex_f *array, double t, int len);
int find_falling_trigger(const complex_f *array, double t, int len);

void firLowpass(double fc, int n, float *kernel);
void flip_array_i(double *srcDst, int len);

// Returns a frequency by counting zero-crossings of a vector
// Assumes no DC/offset is present, aka waveform center line at zero
// Provide a start index
template<class FloatType>
double getAudioFreq(const std::vector<FloatType> &src,
                    double sampleRate,
                    typename std::vector<FloatType>::size_type start_ix = 0)
{
    if(start_ix >= src.size()) return 0.0;

    FloatType lastVal = (FloatType)1.0;
    double lastCrossing, firstCrossing = 0.0;
    bool crossed = false;
    int crossCounter = 0;

    for(auto i = start_ix; i < src.size(); i++) {
        if(src[i] > 0.0 && lastVal < 0.0) {
            double crossLerp = double(i-1) + (-lastVal / (src[i] - lastVal));
            if(crossed) {
                lastCrossing = crossLerp;
                crossCounter++;
            } else {
                crossed = true;
                firstCrossing = crossLerp;
            }
        }
        lastVal = src[i];
    }

    if(crossCounter == 0) {
        return 0.0;
    }

    return sampleRate /
            ((lastCrossing - firstCrossing) / crossCounter);
}

template<class InType, class OutType>
void downsample(std::vector<InType> &src, std::vector<OutType> &dst, int amount)
{
    for(int i = 0; i < src.size(); i += amount) {
        dst.push_back(src[i]);
    }
}

// Bandwidth limit of 0.0003 for single precision
template<class FloatType>
void iirBandPass(const FloatType *input, FloatType *output, double center, double width, int len)
{
    assert(input && output);

    double R = 1.0 - (3.0*width);
    double K = (1.0 - 2.0*R*cos(BB_TWO_PI*center) + R*R) /
            (2.0 - 2.0*cos(BB_TWO_PI*center));
    double a0 = 1.0 - K;
    double a1 = 2.0*(K - R)*cos(BB_TWO_PI*center);
    double a2 = R*R - K;
    double b1 = 2.0*R*cos(BB_TWO_PI*center);
    double b2 = -(R*R);

    if(len <= 3) return;

    output[0] = a0*input[0];
    output[1] = a0*input[1] + a1*input[0] + b1*output[0];
    for(int i = 2; i < len; i++) {
        output[i] = a0*input[i] + a1*input[i-1] + a2*input[i-2] +
                b1*output[i-1] + b2*output[i-2];
    }
}

template<class FloatType>
void iirBandReject(const FloatType *input, FloatType *output, double center, double width, int len)
{
    assert(input && output);

    double R = 1.0 - (3.0*width);
    double K = (1.0 - 2.0*R*cos(BB_TWO_PI*center) + R*R) /
            (2.0 - 2.0*cos(BB_TWO_PI*center));
    double a0 = K;
    double a1 = -2.0*K*cos(BB_TWO_PI*center);
    double a2 = K;
    double b1 = 2.0*R*cos(BB_TWO_PI*center);
    double b2 = -(R*R);

    if(len <= 3) return;

    output[0] = a0*input[0];
    output[1] = a0*input[1] + a1*input[0] + b1*output[0];
    for(int i = 2; i < len; i++) {
        output[i] = a0*input[i] + a1*input[i-1] + a2*input[i-2] +
                b1*output[i-1] + b2*output[i-2];
    }
}

template<class FloatType>
void iirHighPass(const FloatType *input, FloatType *output, int len)
{
    Q_ASSERT(input && output);

    // 0.00128 - 50 Hz
    double a0 = 0.98552035899432355;
    double a1 = -1.9710407179886471;
    double a2 = 0.98552035899432355;
    double b1 = 1.9731243435450900;
    double b2 = -0.97349819497429058;

    // 0.00768 - 300 Hz
//    double a0 = 0.98859879054251221;
//    double a1 = -1.9771975810850244;
//    double a2 = 0.98859879054251221;
//    double b1 = 1.9793646569776204;
//    double b2 = -0.97958579259959844;


    if(len <= 3) return;

    output[0] = a0*input[0];
    output[1] = a0*input[1] + a1*input[0] + b1*output[0];
    for(int i = 2; i < len; i++) {
        output[i] = a0*input[i] + a1*input[i-1] + a2*input[i-2] +
                b1*output[i-1] + b2*output[i-2];
    }
}

// FFT complex-to-complex
// Blackman window
class FFT {
    typedef std::complex<float> kiss_cplx;
    typedef kissfft<float> fft_32f;
public:
    FFT(int len, bool inverse = false) :
        fft_length(len)
    {
        fft_state = std::unique_ptr<fft_32f>(new fft_32f(fft_length, inverse));
        window.resize(fft_length);
        work.resize(fft_length);
        build_flattop_window(&window[0], fft_length);
    }
    ~FFT() {}

    int Length() const { return fft_length; }
    void Transform(const complex_f *input, complex_f *output)
    {
        simdMul_32fc(input, &window[0], &work[0], fft_length); // Window data
        fft_state->transform((kiss_cplx*)(&work[0]), (kiss_cplx*)&output[0]);
        simdCopy_32fc(&output[fft_length/2], &work[0], fft_length/2);
        simdCopy_32fc(&output[0], &output[fft_length/2], fft_length/2);
        simdCopy_32fc(&work[0], &output[0], fft_length/2);
    }

private:
    std::unique_ptr<fft_32f> fft_state;
    std::vector<complex_f> window;
    std::vector<complex_f> work;
    int fft_length;
};

// Filter for single channel signal input
class FirFilter {
public:
    FirFilter(double fc, int filter_len);
    ~FirFilter();

    int Order() const { return order; }
    int Delay() const { return order / 2; }
    void Filter(const float *in, float *out, int n);
    void Reset();

private:
    float *kernel;
    float *overlap; // 2 * len
    int order; // Kernel Length
    double cutoff; // Lowpass freq
};

#endif // BB_LIB_H
