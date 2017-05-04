#ifndef PHASE_NOISE_PLOT_H
#define PHASE_NOISE_PLOT_H

#include "gl_sub_view.h"

class PhaseNoisePlot : public GLSubView {
    Q_OBJECT

public:
    PhaseNoisePlot(Session *sPtr, QWidget *parent = 0);
    ~PhaseNoisePlot();

    void SetDecades(int start, int stop)
    {
        startDecade = start;
        stopDecade = stop;
    }

    //Trace trace;

protected:
    void resizeEvent(QResizeEvent *);
    void mousePressEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);

private:
    void BuildGraticule();
    void DrawTraces();
    void DrawTrace(const Trace *t, const GLVector &v);
    void DrawGratText(QPainter &p);
    void DrawMarkers();
    void DrawMarker(int x, int y, int num);
    void DrawDeltaMarker(int x, int y, int num);

    bool PointInGrat(const QPoint &p) const {
        return QRect(grat_ul.x(), height() - grat_ul.y(),
                     grat_sz.x(), grat_sz.y()).contains(p);
    }

    GLVector traces[TRACE_COUNT]; // Normalized traces
    GLuint traceBufferObject;
    GLVector normalizedTrace;

    GLFont textFont, divFont;
    int startDecade, stopDecade;

    QTime time;

private:
    Q_DISABLE_COPY(PhaseNoisePlot)
};

#endif // PHASE_NOISE_PLOT_H
