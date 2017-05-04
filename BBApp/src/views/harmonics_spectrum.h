#ifndef HARMONICS_SPECTRUM_H
#define HARMONICS_SPECTRUM_H

#include "gl_sub_view.h"
#include "model/trace.h"

class HarmonicsSpectrumPlot : public GLSubView {
    Q_OBJECT

public:
    HarmonicsSpectrumPlot(Session *sPtr, QWidget *parent = 0);
    ~HarmonicsSpectrumPlot();

    Trace harmonics[5];

protected:
    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);

private:
    void DrawTrace(const Trace *t, const GLVector &v, GLuint vbo);
    void DrawTraces();
    void DrawGratText(QPainter &p);
    void DrawMarker(int x, int y, int num);
    void DrawMarkers();

    GLuint traceBufferObjects[5];
    GLVector normalizedTraces[5];
    GLFont textFont, divFont;

    Marker markers[5];
    double markerReadings[5];

private:
    Q_DISABLE_COPY(HarmonicsSpectrumPlot)
};

#endif // HARMONICS_SPECTRUM_H
