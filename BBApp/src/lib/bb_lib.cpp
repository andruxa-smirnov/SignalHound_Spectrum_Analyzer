#include "bb_lib.h"
#include "../model/trace.h"

#include <iostream>

#include <QSize>
#include <QVector>
#include <QStandardPaths>
#include <QDir>
#include <QImage>
#include <QFileDialog>
#include <QSettings>

const QString DEFAULT_IMAGE_SAVE_DIR_KEY("DefaultImageSaveDirectory");
const QString DEFAULT_EXPORT_SAVE_DIR_KEY("DefaultExportSaveDirectory");
const QString DEFAULT_RECORD_SAVE_DIR_KEY("DefaultRecordingSaveDirectory");

char *persist_vs =
        //"#version 110 \n"
        "void main() \n"
        "{ \n"
          "gl_Position = ftransform(); \n"
          "gl_TexCoord[0] = gl_MultiTexCoord0; \n"
        "} \n"
        ;

char *persist_fs =
        //"#version 110 \n"
        "uniform sampler2D texel; \n"
        "void main() \n"
        "{ \n"
            // Get texel value
            "vec4 px = texture2D( texel, gl_TexCoord[0].xy ); \n"

            // Get luminance 0-1, all color channels have same value
            "float L = px.x; \n"

            // Determine colors
            "float R = clamp( (L-0.5)*4.0,       0.0, 1.0); \n"
            "float MG = clamp( 4.0*L - 3.0,      0.0, 1.0 ); \n"
            "float G =  clamp( 4.0*L,            0.0, 1.0 ) - MG; \n"
            "float B = clamp( (0.5-L)*4.0,       0.0, 1.0); \n"
            "float A =  px.w; \n"

            // Output frag color
            "gl_FragColor = vec4( R,G,B,A ); \n"
        "} \n"
        ;

char *dot_persist_temp_vs =
        //"#version 110 \n"
        "void main() \n"
        "{ \n"
          "gl_Position = ftransform(); \n"
          "gl_TexCoord[0] = gl_MultiTexCoord0; \n"
        "} \n"
        ;

char *dot_persist_temp_fs =
        //"#version 110 \n"
        "uniform sampler2D texel; \n"
        "uniform float scale; \n"
        "void main() \n"
        "{ \n"
            // Get texel value
            "vec4 px = texture2D( texel, gl_TexCoord[0].xy ); \n"

            // Get luminance 0-1, all color channels have same value
            "float L = px.x * scale; \n"

            // Determine colors
            "float R = clamp( (L-0.5)*4.0,       0.0, 1.0); \n"
            "float MG = clamp( 4.0*L - 3.0,      0.0, 1.0 ); \n"
            "float G =  clamp( 4.0*L,            0.0, 1.0 ) - MG; \n"
            "float B = clamp( (0.5-L)*4.0,       0.0, 1.0); \n"
            "float A =  px.w; \n"

            // Output frag color
            "gl_FragColor = vec4( R,G,B,A ); \n"
        "} \n"
        ;

// Native bw look up table
bandwidth_lut native_bw_lut[] = {
    { 0.301003456, 536870912 },  //0
    { 0.602006912, 268435456 },  //1
    { 1.204013824, 134217728 },  //2
    { 2.408027649, 67108864 },   //3
    { 4.816055298, 33554432 },   //4
    { 9.632110596, 16777216 },   //5
    { 19.26422119, 8388608 },    //6
    { 38.52844238, 4194304 },    //7
    { 77.05688477, 2097152 },    //8
    { 154.1137695, 1048576 },    //9
    { 308.2275391, 524288 },     //10
    { 616.4550781, 262144 },     //11
    { 1232.910156, 131072 },     //12
    { 2465.820313, 65536 },      //13 rt streaming end
    { 4931.640625, 32768 },      //14
    { 9863.28125,  16384 },      //15
    { 19726.5625,  8192 },       //16
    { 39453.125,   4096 },       //17
    { 78906.25,    2048 },       //18
    { 157812.5,    1024 },       //19
    { 315625,      512 },        //20
    { 631250,      256 },        //21 rt streaming begin
    { 1262500,     128 },        //22
    { 2525000,     64 },         //23
    { 5050000,     32 },         //24
    { 10100000 ,   16 }          //25
};
const int native_bw_lut_sz =
        sizeof(native_bw_lut) / sizeof(bandwidth_lut);

span_to_bandwidth auto_bw_lut[] = {
    // span    native      non-native
    { 1.0e9,   315625,       3.0e5 },
    { 500.0e6, 157812.5,     1.0e5 },
    { 200.0e6, 78906.25,     1.0e5 },
    { 100.0e6, 39453.125,    3.0e4 },
    { 50.0e6,  19726.5625,   3.0e4 },
    { 20.0e6,  9863.28125,   1.0e4 },
    { 10.0e6,  4931.640625,  1.0e4 },
    { 5.0e6,   2465.820313,  3.0e3 },
    { 2.0e6,   1232.910156,  1.0e3 },
    { 1.0e6,   616.4550781,  1.0e3 },
    { 0.5e6,   308.2275391,  3.0e2 },
    { 0.2e6,   154.1137695,  1.0e2 }
};
const int auto_bw_lut_sz =
        sizeof(auto_bw_lut) / sizeof(span_to_bandwidth);

sa_span_to_bandwidth sa_auto_bw_lut[] = {
    { 1.0e9,   1.0e5 },
    { 500.0e6, 1.0e5 },
    { 200.0e6, 1.0e5 },
    { 100.0e6, 3.0e4 },
    { 50.0e6,  3.0e4 },
    { 20.0e6,  1.0e4 },
    { 10.0e6,  1.0e4 },
    { 5.0e6,   3.0e3 },
    { 2.0e6,   1.0e3 },
    { 1.0e6,   1.0e3 },
    { 0.5e6,   3.0e2 },
    { 0.2e6,   1.0e2 }
};
const int sa_auto_bw_lut_sz =
        sizeof(sa_auto_bw_lut) / sizeof(sa_span_to_bandwidth);

GLShader::GLShader(GLShaderType type, char *file_name)
    : compiled(GL_FALSE),
      shader_source(nullptr),
      shader_type(type),
      shader_handle(0)
{
    shader_source = file_name; //bb_lib::get_gl_shader_source(file_name);
}

GLShader::~GLShader()
{
    if(shader_source) {
        //delete [] shader_source;
    }
}

bool GLShader::Compile(QOpenGLFunctions *gl)
{    
    if(compiled == GL_TRUE) {
        return true;
    }

    if(!shader_source) {
        return false;
    }

    if(shader_type == VERTEX_SHADER)
        shader_handle = gl->glCreateShader(GL_VERTEX_SHADER);
    else if(shader_type == FRAGMENT_SHADER)
        shader_handle = gl->glCreateShader(GL_FRAGMENT_SHADER);

    gl->glShaderSource(shader_handle, 1, (const char**)(&shader_source), 0);
    gl->glCompileShader(shader_handle);
    gl->glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &compiled);

    return Compiled();
}

GLProgram::GLProgram(char *vertex_file,
                     char *fragment_file)
    : vertex_shader(VERTEX_SHADER, vertex_file),
      fragment_shader(FRAGMENT_SHADER, fragment_file),
      compiled(GL_FALSE)
{

}

GLProgram::~GLProgram()
{

}

bool GLProgram::Compile(QOpenGLFunctions *gl)
{
    if(compiled == GL_TRUE) {
        return false;
    }

    program_handle = gl->glCreateProgram();

    vertex_shader.Compile(gl);
    fragment_shader.Compile(gl);

    gl->glAttachShader(program_handle, vertex_shader.Handle());
    gl->glAttachShader(program_handle, fragment_shader.Handle());

    //std::cout << "program handle : " << program_handle << std::endl;
    //std::cout << "vertex handle : " << vertex_shader.Handle() << std::endl;
    //std::cout << "fragment handle : " << fragment_shader.Handle() << std::endl;

    //int log_len;
    //char log[1000];
    //gl->glGetProgramInfoLog(program_handle, 999, &log_len, log);
    //gl->glGetShaderInfoLog(fragment_shader.Handle(), 999, &log_len, log);
    //std::cout << log << std::endl;


    // Link Program
    gl->glLinkProgram(program_handle);
    gl->glGetProgramiv(program_handle, GL_LINK_STATUS, &compiled);

    //gl->glGetProgramInfoLog(program_handle, 999, &log_len, log);
    //std::cout << log << std::endl;

    //std::cout << "Program Compiled " << compiled << std::endl;
    //if(compiled != GL_TRUE)
    //    exit(-1);

    return Compiled();
}

GLuint get_texture_from_file(const QString &file_name)
{
    QImage image(file_name);

    unsigned char *my_img = (unsigned char*)malloc(image.width() * image.height() * 3);
    unsigned char *img_cpy = image.bits();

    int ix = 0;
    for(int i = 0; i < image.width() * image.height() * 4; i += 4) {
        my_img[ix++] = img_cpy[i];
        my_img[ix++] = img_cpy[i+1];
        my_img[ix++] = img_cpy[i+2];
    }

    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width(), image.height(), 0,
        GL_RGB, GL_UNSIGNED_BYTE, (unsigned char*)my_img);

    free(my_img);

    return t;
}

double getSignalFrequency(const std::vector<complex_f> &src, double sampleRate)
{
    if(src.size() <= 2) {
        return 0.0;
    }

    double phaseToFreq = sampleRate / BB_TWO_PI;
    double phase = atan2(src[0].im, src[0].re);
    double lastPhase = phase;
    double freq = 0.0;

    for(std::vector<complex_f>::size_type i = 1; i < src.size(); i++) {
        phase = atan2(src[i].im, src[i].re);
        double delPhase = phase - lastPhase;
        lastPhase = phase;

        if(delPhase > BB_PI) delPhase -= BB_TWO_PI;
        if(delPhase < (-BB_PI)) delPhase += BB_TWO_PI;

        freq += delPhase;
    }

    freq /= src.size();
    return freq * phaseToFreq;
}

QString getSampleRateString(double sampleRate)
{
    Q_ASSERT(sampleRate >= 0);

    if(sampleRate > 1.0e6) {
        return QString().sprintf("%.3f MS/s", sampleRate / 1.0e6);
    } else if(sampleRate > 1.0e3) {
        return QString().sprintf("%.3f kS/s", sampleRate / 1.0e3);
    }

    return QString().sprintf("%.3f S/s", sampleRate);
}

QString getPreciseTimeString(double seconds)
{
    QString str;
    bool negative = false;

    if(seconds < 0.0) {
        negative = true;
        seconds *= -1.0;
    }

    if(seconds > 1.0) {
        str.sprintf("%.3f s", seconds);
    } else if(seconds > 1.0e-3) {
        str.sprintf("%.3f ms", seconds * 1.0e3);
    } else {
        str.sprintf("%.3f us", seconds * 1.0e6);
    }

    if(negative) {
        str.push_front(QChar('-'));
    }

    return str;
}

void getPeakCorrelation(const complex_f *src,
                        int len,
                        double centerIn,
                        double &centerOut,
                        double &peakPower,
                        double sampleRate)
{
    centerOut = centerIn;

    const int STEPS = 401;

    double stepSize = (0.5 / sampleRate); //312500.0);
    double startFreq = centerIn - stepSize * (STEPS/2 - 1);
    double maxPower = -1.0;
    double maxCenter = 0.0;

    complex_f sum = {0.0, 0.0};
    complex_f v;

    for(int steps = 0; steps < STEPS; steps++) {
        double freq = startFreq + (stepSize * steps);

        for(int i = 0; i < len; i++) {
            v.re = cos(BB_TWO_PI * -freq * i);
            v.im = sin(BB_TWO_PI * -freq * i);

            sum.re += src[i].re * v.re - src[i].im * v.im;
            sum.im += src[i].re * v.im + src[i].im * v.re;
        }

        sum.re /= len;
        sum.im /= len;
        double power = sum.re * sum.re + sum.im * sum.im;

        if(power > maxPower) {
            maxCenter = freq;
            maxPower = power;
        }
    }

    centerOut = maxCenter;
    peakPower = 10.0 * log10(maxPower);
}

int bb_lib::cpy_16u(const ushort *src, ushort *dst, int maxCopy)
{
    int loc = 0;
    while((src[loc] != '\0') && (loc < (maxCopy-1))) {
        dst[loc] = src[loc];
        loc++;
    }
    dst[loc] = '\0';
    return loc;
}

int bb_lib::get_native_bw_index(double bw)
{
    int ix = 0;
    double dif = fabs(bw - native_bw_lut[ix++].bw);
    double ref = fabs(bw - native_bw_lut[ix].bw);

    while(ref < dif) {
        dif = ref;
        ref = fabs(bw - native_bw_lut[++ix].bw);
        if(ix >= (native_bw_lut_sz - 1))
            break;
    }

    return ix - 1;
}

double bb_lib::sa44_sequence_bw(double bw, bool native_bw, bool increase)
{
    int factor = 0;

    while(bw >= 10.0) {
        bw /= 10.0;
        factor++;
    }

    if(increase) {
        if(bw >= 3.0) bw = 10.0;
        else bw = 3.0;
    } else {
        if(bw <= 1.0) bw = 0.3;
        else if(bw <= 3.0) bw = 1.0;
        else bw = 3.0;
    }

    while(factor--) {
        bw *= 10.0;
    }

    return bw;
}

double bb_lib::sa124_sequence_bw(double bw, bool native_bw, bool increase)
{
    int factor = 0;

    if(bw > 250.0e3 && !increase) return 250.0e3;
    if(bw >= 250.0e3 && increase) return 6.0e6;
    if(bw >= 100.0e3 && bw < 250.0e3 && increase) return 250.0e3;
    if(bw <= 250.0e3 && bw > 100.0e3 && !increase) return 100.0e3;

    while(bw >= 10.0) {
        bw /= 10.0;
        factor++;
    }

    if(increase) {
        if(bw >= 3.0) bw = 10.0;
        else bw = 3.0;
    } else {
        if(bw <= 1.0) bw = 0.3;
        else if(bw <= 3.0) bw = 1.0;
        else bw = 3.0;
    }

    while(factor--) {
        bw *= 10.0;
    }

    return bw;
}

// Find the next bandwidth in sequence
// If native, use table
// If non-native, use 1/3/10 sequence
double bb_lib::bb_sequence_bw(double bw, bool native_bw, bool increase)
{
    if(native_bw) {
        int ix = get_native_bw_index(bw);

        if(increase) {
            ix++;
        } else {
            ix--;
        }

        clamp(ix, 0, native_bw_lut_sz);
        bw = native_bw_lut[ix].bw;

    } else { // (!native_bw)
        int factor = 0;

        while(bw >= 10.0) {
            bw /= 10.0;
            factor++;
        }

        if(increase) {
            if(bw >= 3.0) bw = 10.0;
            else bw = 3.0;
        } else {
            if(bw <= 1.0) bw = 0.3;
            else if(bw <= 3.0) bw = 1.0;
            else bw = 3.0;
        }

        while(factor--) {
            bw *= 10.0;
        }
    }

    return bw;
}

double bb_lib::sequence_span(double span, bool increase)
{
    int factor = 0;

    while(span >= 10.0) {
        span /= 10.0;
        factor++;
    }

    if(increase) {
        if(span >= 5.0) {
            span = 10.0;
        } else if(span >= 2.0) {
            span = 5.0;
        } else {
            span = 2.0;
        }
    } else {
        if(span <= 1.0) {
            span = 0.5;
        } else if(span <= 2.0) {
            span = 1.0;
        } else if(span <= 5.0) {
            span = 2.0;
        } else {
            span = 5.0;
        }
    }

    while(factor--) {
        span *= 10.0;
    }

    return span;
}

double bb_lib::adjust_rbw_on_span(const SweepSettings *ss)
{
    double rbw = ss->RBW();

    while((ss->Span() / rbw) > 500000) {
        rbw = bb_sequence_bw(rbw, ss->NativeRBW(), true);
    }

    while((ss->Span() / rbw) < 1.0) {
        rbw = bb_sequence_bw(rbw, ss->NativeRBW(), false);
    }

    return rbw;
}

double bb_lib::sa44a_adjust_rbw_on_span(const SweepSettings *ss)
{
    double rbw = ss->RBW();

    // Limit points in sweep
    while((ss->Span() / rbw) > 500000) {
        rbw = sa44_sequence_bw(rbw, false, true);
    }
    while((ss->Span() / rbw) < 1.0) {
        rbw = sa44_sequence_bw(rbw, false, false);
    }

    if(ss->Mode() != MODE_SWEEPING) {
        return rbw;
    }

    // Do some clamping at the upper end
    if(rbw > 100.0e3) {
        if(rbw > 150.0e3) {
            rbw = 250.0e3;
        } else {
            rbw = 100.0e3;
        }
    }

    // Min RBW limitation for SA44A
    if(ss->Span() > 200.0e3 && rbw < 6.5e3) {
        rbw = 6.5e3;
    }

    // RBW limitation at low freq
    if(ss->Start() < 16.0e6 && ss->Span() > 200.0e3) {
        if(rbw < 6.5e3) {
            rbw = 6.5e3;
        }
    }

    if(rbw < 0.1) {
        rbw = 0.1;
    }

    return rbw;
}

double bb_lib::sa44b_adjust_rbw_on_span(const SweepSettings *ss)
{
    double rbw = ss->RBW();

    // Limit points in sweep
    while((ss->Span() / rbw) > 500000) {
        rbw = sa44_sequence_bw(rbw, false, true);
    }
    while((ss->Span() / rbw) < 1.0) {
        rbw = sa44_sequence_bw(rbw, false, false);
    }

    if(ss->Mode() != MODE_SWEEPING) {
        return rbw;
    }

    // clamp to 100kHz or 250kHz at the upper end
    if(rbw > 100.0e3) {
        if(rbw > 150.0e3) {
            rbw = 250.0e3;
        } else {
            rbw = 100.0e3;
        }
    }

    // RBW limitation at 100MHz
    if(ss->Span() > 98.0e6 && rbw < 6.5e3) {
        rbw = 6.5e3;
    }

    // RBW limitation at low freq
    if(ss->Start() < 16.0e6 && ss->Span() > 200.0e3) {
        if(rbw < 6.5e3) {
            rbw = 6.5e3;
        }
    }

    if(rbw < 0.1) {
        rbw = 0.1;
    }

    // Mid-range sweep, limit RBW to a specific sweep time
    // All mid-range sweeps, limit to 30Hz
    if(ss->Span() > 200.0e3
            && ss->Span() < 98.0e6
            && ss->Start() > 16.0e6) {
        double limitRBW = ss->Span() / 80.0e3;
        if(rbw < limitRBW) rbw = limitRBW;
        if(rbw < 30.0) rbw = 30.0;
    }

    return rbw;
}

double bb_lib::sa124_adjust_rbw_on_span(const SweepSettings *ss)
{
    double rbw = ss->RBW();

    // Broadband sweep for 124 limitations
    if((ss->Start() < 200.0e6 || ss->Span() < 200.0e6) && ss->RBW() > 250.0e3) {
        rbw = 250.0e3;
    }

    // Limit points in sweep
    while((ss->Span() / rbw) > 500000) {
        rbw = sa124_sequence_bw(rbw, false, true);
    }
    while((ss->Span() / rbw) < 1.0) {
        rbw = sa124_sequence_bw(rbw, false, false);
    }

    if(ss->Mode() != MODE_SWEEPING) {
        return rbw;
    }

    // Do some clamping at the upper end either to 100kHz or 250kHz
    if(rbw > 100.0e3) {
        if(rbw > 150.0e3) {
            if(rbw > 275.0e3) {
                rbw = 6.0e6;
            } else {
                rbw = 250.0e3;
            }
        } else {
            rbw = 100.0e3;
        }
    }

    // RBW limitation above 100MHz span
    if(ss->Span() > 98.0e6 && rbw < 6.5e3) {
        rbw = 6.5e3;
    }

    // Low freq RBW limitation
    if(ss->Start() < 16.0e6 && ss->Span() > 200.0e3) {
        if(rbw < 6.5e3) {
            rbw = 6.5e3;
        }
    }

    if(rbw < 0.1) {
        rbw = 0.1;
    }

    // Mid-range sweep, limit RBW to a specific sweep time
    // All mid-range sweeps, limit to 30Hz
    if(ss->Span() > 200.0e3 && ss->Span() < 98.0e6 && ss->Start() > 16.0e6) {
        double limitRBW = ss->Span() / 80.0e3;
        if(rbw < limitRBW) rbw = limitRBW;
        if(rbw < 30.0) rbw = 30.0;
    }

    return rbw;
}

// Auto RBW based on span
double bb_lib::get_best_rbw(const SweepSettings *ss)
{
    int best_ix = 0;
    double best_diff = fabs(ss->Span() - auto_bw_lut[0].span);

    for(int i = 1; i < auto_bw_lut_sz; i++) {
        double d = fabs(ss->Span() - auto_bw_lut[i].span);
        if(d < best_diff) {
            best_diff = d;
            best_ix = i;
        }
    }

    if(ss->NativeRBW()) {
        return auto_bw_lut[best_ix].nbw;
    }

    return auto_bw_lut[best_ix].nnbw;
}

double bb_lib::sa_get_best_rbw(const SweepSettings *ss)
{
    int best_ix = 0;
    double best_diff = fabs(ss->Span() - sa_auto_bw_lut[0].span);

    for(int i = 1; i < sa_auto_bw_lut_sz; i++) {
        double d = fabs(ss->Span() - sa_auto_bw_lut[i].span);
        if(d < best_diff) {
            best_diff = d;
            best_ix = i;
        }
    }

    double rbw = sa_auto_bw_lut[best_ix].rbw;

    if(ss->Mode() != MODE_SWEEPING) {
        return rbw;
    }

    if(ss->Start() < 16.0e6 && ss->Span() > 200.0e3) {
        if(rbw < 6.5e3) {
            return 6.5e3;
        }
    }

    return rbw;
}

double bb_lib::sa44a_get_best_rbw(const SweepSettings *ss)
{
    int best_ix = 0;
    double best_diff = fabs(ss->Span() - sa_auto_bw_lut[0].span);

    for(int i = 1; i < sa_auto_bw_lut_sz; i++) {
        double d = fabs(ss->Span() - sa_auto_bw_lut[i].span);
        if(d < best_diff) {
            best_diff = d;
            best_ix = i;
        }
    }

    double rbw = sa_auto_bw_lut[best_ix].rbw;

    if(ss->Mode() != MODE_SWEEPING) {
        return rbw;
    }

    if(ss->Span() > 200.0e3 && rbw < 6.5e3) {
        rbw = 6.5e3;
    }

    if(ss->Start() < 16.0e6 && ss->Span() > 200.0e3) {
        if(rbw < 6.5e3) {
            return 6.5e3;
        }
    }

    return rbw;
}

// Get Users MyDocuments path, append application directory
QString bb_lib::get_my_documents_path()
{
    QString path = QStandardPaths::
            writableLocation(QStandardPaths::DocumentsLocation);
    assert(path.length() > 0);
    path += "/SignalHound/";
    QDir().mkdir(path);
    return path;
}

char* bb_lib::get_gl_shader_source(const char *file_name)
{
    FILE *f = NULL;
    fopen_s(&f, file_name, "rb");
    if(f == NULL) {
        return 0; // Unable to open file
    }
    fseek(f, 0L, SEEK_END);
    int len = ftell(f);
    rewind(f);
    char *source = new char[len+1];
    if(fread(source, 1, len, f) != size_t(len)) {
        return 0; // Unable to read in file
    }
    source[len] = '\0';
    fclose(f);
    return source;
}

QString bb_lib::getUserDirectory(const QString &path)
{
    return QFileDialog::getExistingDirectory(0, "Select Record Directory", path);
}

const QString sh::GetDefaultImageDirectory()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "SignalHound", "Preferences");

    return s.value(DEFAULT_IMAGE_SAVE_DIR_KEY, bb_lib::get_my_documents_path()).toString();
}

void sh::SetDefaultImageDirectory(const QString &dir)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "SignalHound", "Preferences");

    s.setValue(DEFAULT_IMAGE_SAVE_DIR_KEY, dir);
}

const QString sh::GetDefaultExportDirectory()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "SignalHound", "Preferences");

    return s.value(DEFAULT_EXPORT_SAVE_DIR_KEY, bb_lib::get_my_documents_path()).toString();
}

void sh::SetDefaultExportDirectory(const QString &dir)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "SignalHound", "Preferences");

    s.setValue(DEFAULT_EXPORT_SAVE_DIR_KEY, dir);
}

#include <QGLWidget>

// Very small class to test if OpenGL compatible, not used currently
class GLTester : public QGLWidget, public QOpenGLFunctions {
public:
    GLTester() : isOpenGLCompatible(false)
    {
        makeCurrent();
        initializeOpenGLFunctions();

        if(hasOpenGLFeature(QOpenGLFunctions::Buffers)) {
            isOpenGLCompatible = true;
        }

        doneCurrent();
    }

    bool IsOpenGLCompatible() const { return isOpenGLCompatible; }

private:
    bool isOpenGLCompatible;
};

bool sh::isOpenGLCompatible()
{
    GLTester test;
    return test.IsOpenGLCompatible();
}

void normalize_trace(const Trace *t, GLVector &v, QPoint grat_size)
{
    normalize_trace(t,
                    v,
                    grat_size,
                    t->GetSettings()->RefLevel(),
                    t->GetSettings()->Div());
}

void normalize_trace(const Trace *t,
                     GLVector &v,
                     QPoint grat_size,
                     Amplitude refLevel,
                     double dBdiv)
{
    normalize_trace(t->Min(), t->Max(), t->Length(), v,
                    grat_size, refLevel, dBdiv);
}

//// Normalize frequency domain trace
//void normalize_trace(const Trace *t,
//                     GLVector &v,
//                     QPoint grat_size,
//                     Amplitude refLevel,
//                     double dBdiv)
//{
//    v.clear();

//    if(t->Length() <= 0) {
//        return;
//    }

//    // Data needed to transform sweep into a screen trace
//    int currentPix = 0;                                    // Step through all pixels
//    double currentStep = 0.0;                              // Our curr step
//    double step = (float)(grat_size.x()) / ( t->Length() - 1 );   // Step size
//    double ref;                       // Value representing the top of graticule
//    double botRef;                    // Value representing bottom of graticule
//    double xScale = 1.0 / (double)grat_size.x();
//    double yScale;

//    if(refLevel.IsLogScale()) {
//        ref = refLevel.ConvertToUnits(AmpUnits::DBM);
//        botRef = ref - 10.0 * dBdiv;
//        yScale = 1.0 / (10.0 * dBdiv);
//    } else {
//        ref = refLevel.Val();
//        botRef = 0.0;
//        yScale = (1.0 / ref);
//    }

//    v.reserve(grat_size.x() * 4);

//    // Less samples than pixels, create quads max1,min1,max2,min2,max3,min3,..
//    if((int)t->Length() < grat_size.x()) {

//        for(int i = 0; i < (int)t->Length(); i++) {

//            double max = t->Max()[i];
//            double min = t->Min()[i];

//            v.push_back(xScale * (currentStep));
//            v.push_back(yScale * (max - botRef));
//            v.push_back(xScale * (currentStep));
//            v.push_back(yScale * (min - botRef));

//            currentStep += step;
//        }
//        return;
//    }

//    // Keeps track of local max/mins
//    float min = ref;
//    float max = botRef;

//    // More samples than pixels, Create pixel resolution, keeps track
//    //   of min/max for each pixel, draws 1 pixel wide quads
//    for(int i = 0; i < t->Length(); i++) {

//        double minVal = t->Min()[i];
//        double maxVal = t->Max()[i];

//        if(maxVal > max) max = maxVal;
//        if(minVal < min) min = minVal;

//        currentStep += step;

//        if(currentStep > currentPix) {

//            v.push_back(xScale * currentPix);
//            v.push_back(yScale * (min - botRef));
//            v.push_back(xScale * currentPix);
//            v.push_back(yScale * (max - botRef));

//            min = ref;
//            max = botRef;
//            currentPix++;
//        }
//    }
//}

// Normalize frequency domain trace
void normalize_trace(const float *sweepMin,
                     const float *sweepMax,
                     int length,
                     GLVector &v,
                     QPoint grat_size,
                     Amplitude refLevel,
                     double dBdiv)
{
    v.clear();

    if(length <= 0) {
        return;
    }

    // Data needed to transform sweep into a screen trace
    int currentPix = 0;                                    // Step through all pixels
    double currentStep = 0.0;                              // Our curr step
    double step = (float)(grat_size.x()) / ( length - 1 );   // Step size
    double ref;                       // Value representing the top of graticule
    double botRef;                    // Value representing bottom of graticule
    double xScale = 1.0 / (double)grat_size.x();
    double yScale;

    if(refLevel.IsLogScale()) {
        ref = refLevel.ConvertToUnits(AmpUnits::DBM);
        botRef = ref - 10.0 * dBdiv;
        yScale = 1.0 / (10.0 * dBdiv);
    } else {
        ref = refLevel.Val();
        botRef = 0.0;
        yScale = (1.0 / ref);
    }

    v.reserve(grat_size.x() * 4);

    // Less samples than pixels, create quads max1,min1,max2,min2,max3,min3,..
    if(length < grat_size.x()) {

        for(int i = 0; i < length; i++) {

            double max = sweepMax[i];
            double min = sweepMin[i];

            v.push_back(xScale * (currentStep));
            v.push_back(yScale * (max - botRef));
            v.push_back(xScale * (currentStep));
            v.push_back(yScale * (min - botRef));

            currentStep += step;
        }
        return;
    }

    // Keeps track of local max/mins
    float min = ref;
    float max = botRef;

    // More samples than pixels, Create pixel resolution, keeps track
    //   of min/max for each pixel, draws 1 pixel wide quads
    for(int i = 0; i < length; i++) {

        double minVal = sweepMin[i];
        double maxVal = sweepMax[i];

        if(maxVal > max) max = maxVal;
        if(minVal < min) min = minVal;

        currentStep += step;

        if(currentStep > currentPix) {

            v.push_back(xScale * currentPix);
            v.push_back(yScale * (min - botRef));
            v.push_back(xScale * currentPix);
            v.push_back(yScale * (max - botRef));

            min = ref;
            max = botRef;
            currentPix++;
        }
    }
}

//void normalize_trace(const Trace *t, LineList &ll, QSize grat_size)
//{
//    ll.clear();

//    if(t->Length() <= 0) {
//        return;
//    }

//    // Data needed to transform sweep into a screen trace
//    int currentPix = 0;                                   // Step through all pixels
//    double currentStep = 0.0;                              // Our curr step
//    double step = (float)(grat_size.width()) / ( t->Length() - 1 );   // Step size
//    double ref;                       // Value representing the top of graticule
//    double botRef;                    // Value representing bottom of graticule
//    double xScale, yScale;
//    double dBdiv = t->GetSettings()->Div();

//    // Non-FM modulation
//    ref = t->GetSettings()->RefLevel().Val();
//    botRef = ref - 10.0 * dBdiv;
//    yScale = 1.0 / (10.0 * dBdiv);

//    xScale = 1.0 / (float)grat_size.width();

//    ll.reserve(grat_size.width() * 4);

//    // Less samples than pixels, create quads max1,min1,max2,min2,max3,min3,..
//    if((int)t->Length() < grat_size.width()) {

//        for(int i = 0; i < (int)t->Length(); i++) {

//            double max = t->Max()[i];
//            double min = t->Min()[i];

//            ll.push_back(QPointF(xScale * (currentStep),
//                                 yScale * (max - botRef)));
//            ll.push_back(QPointF(xScale * (currentStep),
//                                 yScale * (min - botRef)));

//            currentStep += step;
//        }
//        return;
//    }

//    // Keeps track of local max/mins
//    float min = ref;
//    float max = botRef;

//    // More samples than pixels, Create pixel resolution, keeps track
//    //   of min/max for each pixel, draws 1 pixel wide quads
//    for(int i = 0; i < t->Length(); i++) {

//        double minVal = t->Min()[i];
//        double maxVal = t->Max()[i];

//        if(maxVal > max) max = maxVal;
//        if(minVal < min) min = minVal;

//        currentStep += step;

//        if(currentStep > currentPix) {

//            ll.push_back(QPointF(xScale * currentPix,
//                                 yScale * (min - botRef)));
//            ll.push_back(QPointF(xScale * currentPix,
//                                 yScale * (max - botRef)));

//            min = ref;
//            max = botRef;
//            currentPix++;
//        }
//    }
//}

void build_blackman_window(float *window, int len)
{
    for(int i = 0; i < len; i++) {
        window[i] = 0.42659
                - 0.49656 * cos(2*BB_PI*i/(len-1))
                + 0.076849 * cos(4*BB_PI*i/(len-1));

    }

    double windowAvg = 0.0, invAvg = 0.0;

    for(int i = 0; i < len; i++) {
        windowAvg += window[i];
    }

    invAvg = len / windowAvg;

    for(int i = 0; i < len; i++) {
        window[i] *= invAvg;
    }
}

void build_blackman_window(complex_f *dst, int len)
{
    std::vector<float> temp;
    temp.resize(len);
    build_blackman_window(&temp[0], len);

    for(int i = 0; i < len; i++) {
        dst[i].re = temp[i];
        dst[i].im = 0.0;
    }
}

void build_flattop_window(float *window, int len)
{
    for(int i = 0; i < len; i++) {
        window[i] = 1 - 1.93*cos(2*BB_PI*i/(len-1))
            + 1.29*cos(4*BB_PI*i/(len-1))
            - 0.388*cos(6*BB_PI*i/(len-1))
            + 0.028*cos(8*BB_PI*i/(len-1));
    }

    double windowAvg = 0.0, invAvg = 0.0;

    for(int i = 0; i < len; i++) {
        windowAvg += window[i];
    }

    invAvg = len / windowAvg;

    for(int i = 0; i < len; i++) {
        window[i] *= invAvg;
    }
}

void build_flattop_window(complex_f *dst, int len)
{
    std::vector<float> temp;
    temp.resize(len);
    build_flattop_window(&temp[0], len);

    for(int i = 0; i < len; i++) {
        dst[i].re = temp[i];
        dst[i].im = 0.0;
    }
}

void demod_am(const complex_f *src, float *dst, int len)
{
    for(int i = 0; i < len; i++) {
        dst[i] = src[i].re * src[i].re + src[i].im * src[i].im;
        dst[i] = 10.0 * log10(dst[i]);
    }

    // if(linScale), convert to milliVolts?
//    for(int i = 0; i < len; i++) {
//        dst[i] = 223.6 * sqrt(src[i].re * src[i].re + src[i].im * src[i].im);
//    }
}

//void demod_fm(const complex_f *src, float *dst, int len, double *phase)
//{
//    double lastPhase = (phase) ? (*phase) : 0.0;

//    for(int i = 0; i < len; i++) {
//        double newPhase = atan2(src[i].im, src[i].re);
//        double delPhase = newPhase - lastPhase;
//        lastPhase = newPhase;

//        if(delPhase > BB_PI) delPhase -= BB_TWO_PI;
//        else if(delPhase < (-BB_PI)) delPhase += BB_TWO_PI;

//        dst[i] = delPhase;
//    }

//    for(int i = 0; i < len; i++) {
//        dst[i] *= PHASE_TO_FREQ;
//    }

//    if(phase) *phase = lastPhase;
//}

void demod_fm(const std::vector<complex_f> &src,
              std::vector<float> &dst, double sampleRate)
{
    if(src.size() <= 0) return;

    double phaseToFreq = sampleRate / BB_PI;
    double lastPhase = atan2(src[0].im, src[0].re);

    for(int i = 0; i < src.size(); i++) {
        double newPhase = atan2(src[i].im, src[i].re);
        double delPhase = newPhase - lastPhase;
        lastPhase = newPhase;

        if(delPhase > BB_PI) delPhase -= BB_TWO_PI;
        else if(delPhase < (-BB_PI)) delPhase += BB_TWO_PI;

        dst.push_back(delPhase * phaseToFreq);
    }
}            

int find_rising_trigger(const complex_f *array, double t, int len)
{
    int ix = 0;

    // Must go below trigger level
    while(ix < len) {
        double val = array[ix].re * array[ix].re +
                array[ix].im * array[ix].im;
        if(val < t) break;
        ix++;
    }

    if(ix >= len) return -1;

    while(ix < len) {
        double val = array[ix].re * array[ix].re +
                array[ix].im * array[ix].im;
        if(val > t) return ix;
        ix++;
    }

    return -1;
}

int find_falling_trigger(const complex_f *array, double t, int len)
{
    int ix = 0;

    // Find first sample above trigger
    while(ix < len) {
        double val = array[ix].re * array[ix].re +
                array[ix].im * array[ix].im;
        if(val > t) break;
        ix++;
    }

    if(ix >= len) return -1;

    while(ix < len) {
        double val = array[ix].re * array[ix].re +
                array[ix].im * array[ix].im;
        if(val < t) return ix;
        ix++;
    }

    return -1;
}

// Build complex fir kernel
// dWindow is normalized windowed sinc
// fc is cutoff freq (0 to 0.5), n is kernel size + 1,
// Blackman window a = 0.16
void firLowpass(double fc, int n, float *kernel)
{
    int k;
    int m = (n - 1) / 2; // M is half kernel size
    double avgval = 0.0;

    for(k = 0; k < n; k++) {
        if(k == m) {
            kernel[k] = 2 * BB_PI * fc;
        } else {
            // sinc function
            kernel[k] = sin(BB_TWO_PI * fc * (k - m)) / (k - m);
        }
        kernel[k] *= (0.42 - 0.5 * cos(BB_TWO_PI * k / (n - 1)) +
                       0.08 * cos(BB_FOUR_PI * k / (n - 1)));
        avgval += kernel[k];
    }

    double invavg = 1.0 / avgval;
    for(k = 0; k < n; k++) { // Normalize window
        kernel[k] *= invavg;
    }
}

void flip_array_i(float *srcDst, int len)
{
    if(len <= 1) {
        return;
    }

    int front = 0;
    int back = len - 1;

    while(front > len) {
        double t = srcDst[front];
        srcDst[front] = srcDst[back];
        srcDst[back] = t;

        front++;
        back--;
    }
}

inline void zero_array(float *in, int len) {
    for(int i = 0; i < len; i++) in[i] = 0.0;
}

inline void copy_array(const float *in, float *out, int len) {
    for(int i = 0; i < len; i++) out[i] = in[i];
}

inline double mul_accum(const float *src1, const float *src2, int len) {
    double sum = 0.0;
    for(int i = 0; i < len; i++) {
        sum += (src1[i] * src2[i]);
    }
    return sum;
}

//template<class FloatType>
//void iirBandPass(FloatType *input, FloatType *output, double center, double width, int len)
//{
//    Q_ASSERT(input && output);

//    double R = 1.0 - (3.0*width);
//    double K = (1.0 - 2.0*R*cos(BB_TWO_PI*center) + R*R) /
//            (2.0 - 2.0*cos(BB_TWO_PI*center));
//    double a0 = 1.0 - K;
//    double a1 = 2.0*(K - R)*cos(BB_TWO_PI*center);
//    double a2 = R*R - K;
//    double b1 = 2.0*R*cos(BB_TWO_PI*center);
//    double b2 = -(R*R);

//    if(len <= 3) return;

//    output[0] = a0*input[0];
//    output[1] = a0*input[1] + a1*input[0] + b1*output[0];
//    for(int i = 2; i < len; i++) {
//        output[i] = a0*input[i] + a1*input[i-1] + a2*input[i-2] +
//                b1*output[i-1] + b2*output[i-2];
//    }
//}

//template<class FloatType>
//void iirBandReject(FloatType *input, FloatType *output, double center, double width, int len)
//{
//    Q_ASSERT(input && output);

//    double R = 1.0 - (3.0*width);
//    double K = (1.0 - 2.0*R*cos(BB_TWO_PI*center) + R*R) /
//            (2.0 - 2.0*cos(BB_TWO_PI*center));
//    double a0 = K;
//    double a1 = -2.0*K*cos(BB_TWO_PI*center);
//    double a2 = K;
//    double b1 = 2.0*R*cos(BB_TWO_PI*center);
//    double b2 = -(R*R);

//    if(len <= 3) return;

//    output[0] = a0*input[0];
//    output[1] = a0*input[1] + a1*input[0] + b1*output[0];
//    for(int i = 2; i < len; i++) {
//        output[i] = a0*input[i] + a1*input[i-1] + a2*input[i-2] +
//                b1*output[i-1] + b2*output[i-2];
//    }
//}


FirFilter::FirFilter(double fc, int filter_len)
    : order(filter_len), cutoff(fc)
{
    kernel = new float[order];
    firLowpass(cutoff, order, kernel);
    flip_array_i(kernel, order);

//    QFile *f = new QFile("Filter.txt");
//    f->open(QIODevice::WriteOnly | QIODevice::Text);
//    QTextStream out(f);
//    for(int i = 0; i < order; i++) {
//        out << kernel[i] << "\n";
//    }
//    f->close();
//    delete f;

    overlap = new float[2*order];
    zero_array(overlap, 2*order);
}

FirFilter::~FirFilter()
{
    delete [] kernel;
    delete [] overlap;
}

// Input must be longer than kernel for now
// In-place safe
void FirFilter::Filter(const float *in, float *out, int n)
{
    assert(n > order);

    int ix = 0, eix = 0;

    // Copy initial input into overlap buffer
    copy_array(in, overlap + (order-1), order);

    for(; ix < order - 1; ix++) {
        out[ix] = mul_accum(overlap + ix, kernel, order);
    }

    // Finish filter
    for(; ix < n; ix++, eix++) {
        out[ix] = mul_accum(in + eix, kernel, order);
    }

    // Copy input into overlap
    copy_array(in + (n-(order-1)), overlap, order - 1);
}

void FirFilter::Reset()
{
    zero_array(overlap, 2*order);
}
