#ifndef DEMOD_SPECTRUM_PLOT_H
#define DEMOD_SPECTRUM_PLOT_H

#include "gl_sub_view.h"

class DemodSpectrumPlot : public GLSubView {
    Q_OBJECT

    static const int MAX_FFT_SIZE = 4096;
public:
    DemodSpectrumPlot(Session *sPtr, QWidget *parent = 0);
    ~DemodSpectrumPlot();

protected:
    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);

private:
    void DrawSpectrum();
    void DrawTrace(const GLVector &v);
    void DrawPlotText(QPainter &p);

    std::unique_ptr<FFT> fft;
    std::vector<complex_f> postTransform;

    GLVector spectrum, spectrumToDraw;
    GLuint traceVBO;
    GLFont textFont, divFont;

private:
    DISALLOW_COPY_AND_ASSIGN(DemodSpectrumPlot)
};

#endif // DEMOD_SPECTRUM_PLOT_H
