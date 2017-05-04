#include "phase_noise_plot.h"

#include <QMouseEvent>

PhaseNoisePlot::PhaseNoisePlot(Session *sPtr, QWidget *parent) :
    GLSubView(sPtr, parent),
    textFont(12),
    divFont(12),
    startDecade(2),
    stopDecade(6)
{
    makeCurrent();

    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    context()->format().setDoubleBuffer(true);

    glGenBuffers(1, &traceBufferObject);

    doneCurrent();
}

PhaseNoisePlot::~PhaseNoisePlot()
{
    makeCurrent();

    // Delete GL objects
    glDeleteBuffers(1, &traceBufferObject);

    doneCurrent();
}

void PhaseNoisePlot::resizeEvent(QResizeEvent *)
{
    SetGraticuleDimensions(QPoint(60, 50),
                           QPoint(width() - 80, height() - 100));
}

void PhaseNoisePlot::mousePressEvent(QMouseEvent *e)
{
    if(PointInGrat(e->pos())) {
        // Make point relative to upper left of graticule
        int xPos = e->pos().x() - grat_ul.x();

        if(xPos < 0 || xPos > grat_sz.x()) {
            return;
        }

        GetSession()->trace_manager->PlaceMarkerPercent((double)xPos / grat_sz.x());
    }

    QGLWidget::mousePressEvent(e);
}

void PhaseNoisePlot::paintEvent(QPaintEvent *)
{
    makeCurrent();

    glQClearColor(GetSession()->colors.background);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);

    BuildGraticule();

    SetGraticuleDimensions(QPoint(60, 50),
                           QPoint(width() - 80, height() - 100));

    if(grat_sz.x() >= 600) {
        textFont = GLFont(14);
    } else if(grat_sz.x() <= 350) {
        textFont = GLFont(8);
    } else {
        int mod = (600 - grat_sz.x()) / 50;
        textFont = GLFont(13 - mod);
    }

    // Calculate dimensions for the presence of the title bar
    if(!GetSession()->GetTitle().isNull()) {
        grat_ul -= QPoint(0, 20);
        grat_sz -= QPoint(0, 20);
    }

    glViewport(0, 0, width(), height());

    DrawGraticule();
    DrawTraces();

    glDisable(GL_DEPTH_TEST);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    DrawMarkers();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(0.0, height());
    p.setPen(QPen(GetSession()->colors.text));
    p.setFont(textFont.Font());

    DrawTextQueue(p);
    DrawGratText(p);

    p.end();
    glPopAttrib();

    doneCurrent();
    swapBuffers();
}

void PhaseNoisePlot::DrawTraces()
{
    TraceManager *manager = GetSession()->trace_manager;

    // Prep viewport
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(grat_ll.x(), grat_ll.y(),
               grat_sz.x(), grat_sz.y());

    // Prep modelview
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Ortho
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

    // Nice lines, doesn't smooth quads
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glLineWidth(GetSession()->prefs.trace_width);

    manager->Lock();

    // Loop through each trace
    for(int i = 0; i < TRACE_COUNT; i++) {
        // If Trace is active, normalize and draw it
        const Trace *trace = manager->GetTrace(i);

        if(trace->Active()) {
            normalize_trace(trace,
                            traces[i],
                            grat_sz,
                            GetSession()->sweep_settings->RefLevel().Val(),
                            GetSession()->sweep_settings->Div());
            DrawTrace(trace, traces[i]);
        }
    }

    manager->Unlock();

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

void PhaseNoisePlot::DrawTrace(const Trace *t, const GLVector &v)
{
    if(v.size() < 1) {
        return;
    }

    QColor c = t->Color();
    glColor3f(c.redF(), c.greenF(), c.blueF());

    // Put the trace in the vbo
    glBindBuffer(GL_ARRAY_BUFFER, traceBufferObject);
    glBufferData(GL_ARRAY_BUFFER, v.size()*sizeof(float),
                 &v[0], GL_DYNAMIC_DRAW);
    glVertexPointer(2, GL_FLOAT, 0, INDEX_OFFSET(0));

    // Draw fill
    glDrawArrays(GL_TRIANGLE_STRIP, 0, v.size() / 2);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Draw lines
    glVertexPointer(2, GL_FLOAT, 16, INDEX_OFFSET(0));
    glDrawArrays(GL_LINE_STRIP, 0, v.size() / 4);
    glVertexPointer(2, GL_FLOAT, 16, INDEX_OFFSET(8));
    glDrawArrays(GL_LINE_STRIP, 0, v.size() / 4);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Unbind array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PhaseNoisePlot::BuildGraticule()
{
    int divisions = (stopDecade - startDecade);

    std::vector<float> points;

    for(int i = 0; i <= 10; i++) {
        points.push_back(0.0);
        points.push_back(0.1 * i);
        points.push_back(1.0);
        points.push_back(0.1 * i);
    }

    for(int d = 0; d < divisions; d++) {
        float offset = (float)d / (float)divisions;
        for(int i = 0; i < 10; i++) {
            double x = log10(i) / (double)divisions + offset;
            points.push_back(x);
            points.push_back(0.0);
            points.push_back(x);
            points.push_back(1.0);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, gratVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float),
                 &points[0], GL_DYNAMIC_DRAW);

    innerGratPoints = points.size();
}

void PhaseNoisePlot::DrawGratText(QPainter &p)
{
    QString str;
    const SweepSettings *s = GetSession()->sweep_settings;
    TraceManager *tm = GetSession()->trace_manager;

    int textHeight = textFont.GetTextHeight();

    double div = s->RefLevel().IsLogScale() ? s->Div() : (s->RefLevel().Val() / 10.0);

    str = GetSession()->GetTitle();
    if(!str.isNull()) {
        p.setFont(QFont("Arial", 20));
        DrawString(p, str, width() / 2, height() - 22, CENTER_ALIGNED);
    }

    p.setFont(textFont.Font());
    DrawString(p, "Center " + s->Center().GetFreqString(),
               grat_ll.x() + grat_sz.x()/2, grat_ll.y() - textHeight, CENTER_ALIGNED);
    DrawString(p, "Ref " + s->RefLevel().GetValueString() + "dBc/Hz",
               grat_ll.x()+5, grat_ul.y()+textHeight, LEFT_ALIGNED);
    str.sprintf("Div %.1f", div);
    DrawString(p, str, grat_ul.x()+5, grat_ul.y()+2 , LEFT_ALIGNED);
    s->GetAttenString(str);
    DrawString(p, str, grat_ll.x() + grat_sz.x()/2, grat_ul.y()+2, CENTER_ALIGNED);

    str = QString("Start ") + Frequency(pow(10.0, startDecade+1)).GetFreqString();
    DrawString(p, str, grat_ll.x()+5, grat_ll.y()-textHeight, LEFT_ALIGNED);
    str = QString("Stop ") + Frequency(pow(10.0, stopDecade+1)).GetFreqString();
    DrawString(p, str, grat_ll.x()+grat_sz.x()-5, grat_ll.y()-textHeight, RIGHT_ALIGNED);
    QVariant elapsed = time.restart();
    str.sprintf("%d pts in %d ms", tm->GetTrace(0)->Length(), elapsed.toInt());
    DrawString(p, str, grat_ll.x()+grat_sz.x()-5,
               grat_ll.y()-textHeight*2, RIGHT_ALIGNED);

    // y-axis labels
    p.setFont(divFont.Font());
    for(int i = 0; i <= 8; i += 2) {
        int x_pos = 58, y_pos = (grat_sz.y() / 10) * i + grat_ll.y() - 5;
        QString div_str;
        div_str.sprintf("%.2f", s->RefLevel() - (div*(10-i)));
        DrawString(p, div_str, x_pos, y_pos, RIGHT_ALIGNED);
    }

    p.setFont(textFont.Font());
    if(GetSession()->device->IsOpen()) {
        // Uncal text strings
        bool uncal = false;
        int uncal_x = grat_ul.x() + 5, uncal_y = grat_ul.y() - textHeight;
        glColor3f(1.0, 0.0, 0.0);
        if(!GetSession()->device->IsPowered()) {
            uncal = true;
            DrawString(p, "Low Voltage", uncal_x, uncal_y, LEFT_ALIGNED);
            uncal_y -= textHeight;
        }
        if(GetSession()->device->ADCOverflow()) {
            uncal = true;
            DrawString(p, "IF Overload", uncal_x, uncal_y, LEFT_ALIGNED);
            uncal_y -= textHeight;
        }
        if(GetSession()->device->NeedsTempCal()) {
            uncal = true;
            DrawString(p, "Device Temp", uncal_x, uncal_y, LEFT_ALIGNED);
            uncal_y -= textHeight;
        }
        if(uncal) {
            DrawString(p, "Uncal", grat_ul.x() - 5, grat_ul.y() + 2, RIGHT_ALIGNED);
        }
    }
}

void PhaseNoisePlot::DrawMarkers()
{
    SweepSettings *s = GetSession()->sweep_settings;
    TraceManager *tm = GetSession()->trace_manager;

    // Viewport on grat, full pixel scale
    glPushAttrib(GL_VIEWPORT_BIT);
    //glViewport(grat_ll.x(), grat_ll.y(), grat_sz.x(), grat_sz.y());
    glViewport(0, 0, width(), height());

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    //glOrtho(0, grat_sz.x(), 0, grat_sz.y(), -1, 1);
    glOrtho(0, width(), 0, height(), -1, 1);

    int x_print = grat_ll.x() + grat_sz.x() - 5;
    int y_print = grat_ll.y() + grat_sz.y() - 20;

    //tm->SolveMarkers(s);
    tm->SolveMarkersForPhaseNoise(s);

    // Nice lines, doesn't smooth quads
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glLineWidth(1.0);

    for(int i = 0; i < MARKER_COUNT; i++) {
        Marker *m = tm->GetMarker(i);
        if(!m->Active() || !m->InView()) {
            continue;
        }

        if(m->InView()) {
            DrawMarker(grat_ll.x() + m->xRatio() * grat_sz.x(),
                       grat_ll.y() + m->yRatio() * grat_sz.y(), i + 1);
        }

        if(m->DeltaActive() && m->DeltaInView()) {
            DrawDeltaMarker(grat_ll.x() + m->delxRatio() * grat_sz.x(),
                            grat_ll.y() + m->delyRatio() * grat_sz.y(), i + 1);
        }

        double freq = pow(10.0, (double)(startDecade+1) + (double)m->Index() / 100.0);

        // Does not have to be in view to draw the delta values
        if(m->DeltaActive()) {
            glQColor(GetSession()->colors.text);

            double delFreq = pow(10.0, (double)(startDecade+1) + (double)m->DeltaIndex() / 100.0);
            QString freqStr = Frequency(freq - delFreq).GetFreqString();
            QString ampStr = QString().sprintf("%.2f dB", m->Amp().Val() - m->DeltaAmp());

//            DrawString("Mkr " + QVariant(i+1).toString() + " Delta: " + freqStr + " " + ampStr,
//                       textFont, QPoint(x_print, y_print), RIGHT_ALIGNED);
            AddTextToRender("Mkr " + QVariant(i+1).toString() + " Delta: " + freqStr + " " + ampStr,
                            QPoint(x_print, y_print), RIGHT_ALIGNED, textFont.Font(),
                            GetSession()->colors.text);
            y_print -= 20;
        } else if(m->Active()) {
            glQColor(GetSession()->colors.text);

            QString freqStr = Frequency(freq).GetFreqString();
            QString ampStr = QString().sprintf("%.2f dBc/Hz", m->Amp().Val());

//            DrawString("Mkr " + QVariant(i+1).toString() + ": " + freqStr + " " + ampStr,
//                       textFont, QPoint(x_print, y_print), RIGHT_ALIGNED);
            AddTextToRender("Mkr " + QVariant(i+1).toString() + ": " + freqStr + " " + ampStr,
                            QPoint(x_print, y_print), RIGHT_ALIGNED, textFont.Font(),
                            GetSession()->colors.text);
            y_print -= 20;
        }
    }

    // Disable nice lines
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

void PhaseNoisePlot::DrawMarker(int x, int y, int num)
{
    glQColor(GetSession()->colors.markerBackground);
    glBegin(GL_POLYGON);
    glVertex2f(x, y);
    glVertex2f(x + 10, y + 15);
    glVertex2f(x, y + 30);
    glVertex2f(x - 10, y + 15);
    glEnd();

    glQColor(GetSession()->colors.markerBorder);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x, y);
    glVertex2f(x + 10, y + 15);
    glVertex2f(x, y + 30);
    glVertex2f(x - 10, y + 15);
    glVertex2f(x, y);
    glEnd();

    glQColor(GetSession()->colors.markerText);
    QString str;
    str.sprintf("%d", num);
    AddTextToRender(str, QPoint(x, y+10), CENTER_ALIGNED,
                    divFont.Font(), GetSession()->colors.markerText);
}

void PhaseNoisePlot::DrawDeltaMarker(int x, int y, int num)
{
    glQColor(GetSession()->colors.markerBackground);
    glBegin(GL_POLYGON);
    glVertex2f(x, y);
    glVertex2f(x + 11, y + 11);
    glVertex2f(x + 11, y + 27);
    glVertex2f(x - 11, y + 27);
    glVertex2f(x - 11, y + 11);
    glEnd();

    glQColor(GetSession()->colors.markerBorder);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x, y);
    glVertex2f(x + 11, y + 11);
    glVertex2f(x + 11, y + 27);
    glVertex2f(x - 11, y + 27);
    glVertex2f(x - 11, y + 11);
    glVertex2f(x, y);
    glEnd();

    glQColor(GetSession()->colors.markerText);
    QString str;
    str.sprintf("R%d", num);
    AddTextToRender(str, QPoint(x, y+11), CENTER_ALIGNED,
                    divFont.Font(), GetSession()->colors.markerText);
}

