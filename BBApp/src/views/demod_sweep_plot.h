#ifndef DEMOD_SWEEP_PLOT_H
#define DEMOD_SWEEP_PLOT_H

#include "gl_sub_view.h"
#include "lib/bb_lib.h"
#include "widgets/entry_widgets.h"

#include <QToolBar>

enum DemodType {
    DemodTypeAM = 0,
    DemodTypeFM = 1,
    DemodTypePM = 2
};

//ReceiverStats getReceiverStats(DemodType type, const GLVector &waveform, double sampleRate);
//ReceiverStats getReceiverStats(const IQSweep &sweep);

// AM/FM/PM Demod Plot Window
class DemodSweepPlot : public GLSubView {
    Q_OBJECT

public:
    DemodSweepPlot(Session *session, QWidget *parent = 0);
    ~DemodSweepPlot();

protected:
    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);

private:
    void DemodAndDraw();
    void DrawPlotText(QPainter &p);
    void DrawMarkers();
    void DrawTrace(const GLVector &v);
    void DrawMarker(int x, int y, int num);
    void DrawDeltaMarker(int x, int y, int num);
    void DrawModAnalysisReport(QPainter &p);

    GLFont textFont, divFont;

    // Waveform paired with x-position values
    GLVector trace;
    GLuint traceVBO;

    DemodType demodType;

    bool markerOn, deltaOn;
    float markerVal, deltaVal;
    int markerIndex, deltaIndex;
    QPointF markerPos, deltaPos;

public slots:
    void changeDemod(int newDemod) {
        demodType = (DemodType)newDemod;
        update();
    }

    void disableMarker() {
        markerOn = false;
        update();
    }

    void toggleDelta() {
        deltaOn = !deltaOn;
        if(deltaOn) {
            deltaIndex = markerIndex;
            deltaPos = markerPos;
            deltaVal = markerVal;
        }
        update();
    }

private slots:

signals:

private:
    DISALLOW_COPY_AND_ASSIGN(DemodSweepPlot)
};

#endif // DEMOD_SWEEP_PLOT_H
