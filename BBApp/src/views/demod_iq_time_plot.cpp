#include "demod_iq_time_plot.h"

DemodIQTimePlot::DemodIQTimePlot(Session *session, QWidget *parent) :
    GLSubView(session, parent),
    textFont(14),
    divFont(12),
    traceVBO(0),
    yScale(0.0)
{
    makeCurrent();

    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    context()->format().setDoubleBuffer(true);
    glGenBuffers(1, &traceVBO);

    doneCurrent();
}

DemodIQTimePlot::~DemodIQTimePlot()
{
    makeCurrent();
    glDeleteBuffers(1, &traceVBO);
    doneCurrent();
}

void DemodIQTimePlot::resizeEvent(QResizeEvent *)
{
    SetGraticuleDimensions(QPoint(60, 50),
                           QPoint(width() - 80, height() - 100));
}

void DemodIQTimePlot::paintEvent(QPaintEvent *)
{
    makeCurrent();

    glQClearColor(GetSession()->colors.background);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);

    glViewport(0, 0, width(), height());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if(grat_sz.x() >= 600) {
        textFont = GLFont(14);
    } else if(grat_sz.x() <= 350) {
        textFont = GLFont(8);
    } else {
        int mod = (600 - grat_sz.x()) / 50;
        textFont = GLFont(13 - mod);
    }

    int textHeight = textFont.GetTextHeight() + 2;
    SetGraticuleDimensions(QPoint(60, textHeight*2),
                           QPoint(width() - 80, height() - textHeight*4));

    DrawGraticule();
    DrawIQLines();

    glDisable(GL_DEPTH_TEST);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width(), 0, height(), -1, 1);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(0.0, height());
    p.setPen(QPen(GetSession()->colors.text));
    p.setFont(textFont.Font());

    DrawPlotText(p);

    p.end();
    glPopAttrib();

    doneCurrent();
    swapBuffers();
}

void DemodIQTimePlot::DrawIQLines()
{
    traces[0].clear();
    traces[1].clear();

    const IQSweep &sweep = GetSession()->iq_capture;
    const std::vector<complex_f> &iq = sweep.iq;

    if(sweep.sweepLen <= 4) return;
    if(sweep.iq.size() <= 4) return;

    glColor3f(1.0, 0.0, 0.0);

    for(int i = 0; i < sweep.sweepLen; i++) {
        traces[0].push_back(i);
        traces[0].push_back(iq[i].re);
        traces[1].push_back(i);
        traces[1].push_back(iq[i].im);
    }

    float max = 0.0;
    for(int i = 0; i < sweep.sweepLen; i++) {
        max = bb_lib::max2(max, iq[i].re);
        max = bb_lib::max2(max, iq[i].im);
    }
    yScale = bb_lib::next_multiple_of(0.05, max);
    if(yScale - max < 0.01) yScale += 0.1;
    if(yScale > 1.5) yScale = 0.75;

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(grat_ll.x(), grat_ll.y(), grat_sz.x(), grat_sz.y());
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    //glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    //glLoadIdentity();
    glOrtho(0, sweep.sweepLen - 1, -yScale, yScale, -1, 1);

    // Nice lines
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glLineWidth(GetSession()->prefs.trace_width);

    qglColor(QColor(255, 0, 0));
    DrawTrace(traces[0]);
    qglColor(QColor(0, 255, 0));
    DrawTrace(traces[1]);

    // Disable nice lines
    glLineWidth(1.0);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

void DemodIQTimePlot::DrawTrace(const GLVector &v)
{
    if(v.size() <= 4) {
        return;
    }

    // Put the trace in the vbo
    glBindBuffer(GL_ARRAY_BUFFER, traceVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size()*sizeof(float),
                 &v[0], GL_DYNAMIC_DRAW);
    glVertexPointer(2, GL_FLOAT, 0, INDEX_OFFSET(0));

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_LINE_STRIP, 0, v.size() / 2);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Unbind array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DemodIQTimePlot::DrawPlotText(QPainter &p)
{
    int ascent = textFont.FontMetrics().ascent();
    int textHeight = textFont.GetTextHeight();

    const IQSweep &sweep = GetSession()->iq_capture;
    const DemodSettings *ds = GetSession()->demod_settings;
    QString str;

    p.beginNativePainting();

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, width(), height());
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width(), 0, height(), -1, 1);

    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_QUADS);
    glVertex2i(grat_ul.x() + 5, grat_ul.y() + 2);
    glVertex2i(grat_ul.x() + 5 + ascent, grat_ul.y() + 2);
    glVertex2i(grat_ul.x() + 5 + ascent, grat_ul.y() + 2 + ascent);
    glVertex2i(grat_ul.x() + 5, grat_ul.y() + 2 + ascent);
    glVertex2i(grat_ul.x() + 5, grat_ul.y() + 2);
    glEnd();

    glColor3f(0.0, 1.0, 0.0);
    glBegin(GL_QUADS);
    glVertex2i(grat_ul.x() + 45, grat_ul.y() + 2);
    glVertex2i(grat_ul.x() + 45 + ascent, grat_ul.y() + 2);
    glVertex2i(grat_ul.x() + 45 + ascent, grat_ul.y() + 2 + ascent);
    glVertex2i(grat_ul.x() + 45, grat_ul.y() + 2 + ascent);
    glVertex2i(grat_ul.x() + 45, grat_ul.y() + 2);
    glEnd();

    glQColor(GetSession()->colors.text);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();

    p.endNativePainting();

    DrawString(p, "I", QPoint(grat_ul.x() + 25, grat_ul.y() + 2), LEFT_ALIGNED);
    DrawString(p, "Q", QPoint(grat_ul.x() + 65, grat_ul.y() + 2), LEFT_ALIGNED);

    str = "IF Bandwidth " + Frequency(sweep.descriptor.bandwidth).GetFreqString(3, true);
    DrawString(p, str, QPoint(grat_ll.x() + 5, grat_ll.y() - textHeight), LEFT_ALIGNED);
    str = "Capture Len " + ds->SweepTime().GetString();
    DrawString(p, str, QPoint(grat_ll.x() + grat_sz.x() - 5, grat_ll.y() - textHeight), RIGHT_ALIGNED);
    str = "Sample Rate " + getSampleRateString(sweep.descriptor.sampleRate);
    DrawString(p, str, QPoint(grat_ll.x() + grat_sz.x() - 5, grat_ul.y() + 2), RIGHT_ALIGNED);

    p.setFont(divFont.Font());
    for(int i = 0; i <= 10; i++) {
        int x_loc = grat_ul.x() - 2,
                y_loc = grat_ul.y() - (i * grat_sz.y()/10.0) - 5;
        str.sprintf("%.2f", yScale - (i * yScale/5.0));
        DrawString(p, str, x_loc, y_loc, RIGHT_ALIGNED);
    }
}
