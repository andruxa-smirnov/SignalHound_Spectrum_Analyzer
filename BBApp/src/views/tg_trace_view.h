#ifndef TG_TRACE_VIEW_H
#define TG_TRACE_VIEW_H

#include "gl_sub_view.h"
#include "model/trace.h"

class TGPlot : public GLSubView {
    Q_OBJECT

public:
    TGPlot(Session *sPtr, QWidget *parent = 0);
    ~TGPlot();

    void SetStepSize(double stepSize) { tgStepSize = stepSize; }

protected:
    void resizeEvent(QResizeEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);

private:
    void BuildGraticule();
    void DrawTraces();
    void DrawTrace(const Trace *t, const GLVector &v, GLuint vbo);
    void RenderMarkers();
    void DrawGratText(QPainter &p);
    void DrawMarker(int x, int y, int num);
    void DrawDeltaMarker(int x, int y, int num);

    bool PointInGrat(const QPoint &p) const {
        return QRect(grat_ul.x(), height() - grat_ul.y(),
                     grat_sz.x(), grat_sz.y()).contains(p);
    }

    QTime time; // Used for sweep time display
    GLuint traceVBO;
    GLFont textFont, divFont;
    GLVector traces[TRACE_COUNT];
    double tgStepSize;

private:
    Q_DISABLE_COPY(TGPlot)
};

#endif // TG_TRACE_VIEW_H
