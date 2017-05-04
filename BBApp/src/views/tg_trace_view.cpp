#include "tg_trace_view.h"
#include "mainwindow.h"

#include <QMouseEvent>

TGPlot::TGPlot(Session *sPtr, QWidget *parent) :
    GLSubView(sPtr, parent),
    textFont(12),
    divFont(12),
    tgStepSize(0)
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

TGPlot::~TGPlot()
{
    makeCurrent();
    glDeleteBuffers(1, &traceVBO);
    doneCurrent();
}

void TGPlot::resizeEvent(QResizeEvent *)
{
    SetGraticuleDimensions(QPoint(60, 50),
                           QPoint(width() - 80, height() - 100));
}

void TGPlot::mousePressEvent(QMouseEvent *e)
{
    if(PointInGrat(e->pos())) {
        // Make point relative to upper left of graticule
        int x_pos = e->pos().x() - grat_ul.x();

        if(x_pos < 0 || x_pos > grat_sz.x()) {
            return;
        }

        GetSession()->trace_manager->PlaceMarkerPercent((double)x_pos / grat_sz.x());
    }

    QGLWidget::mousePressEvent(e);
}

void TGPlot::mouseMoveEvent(QMouseEvent *e)
{
    if(PointInGrat(e->pos())) {
        const SweepSettings *s = GetSession()->sweep_settings;
        double x, xScale, y, yScale;

        xScale = s->Span() / grat_sz.x();
        x = s->Start() + xScale * (e->pos().x() - grat_ll.x());

        if(s->RefLevel().IsLogScale()) {
            yScale = (s->Div() * 10.0) / grat_sz.y();
        } else {
            yScale = s->RefLevel() / grat_sz.y();
        }

        y = s->RefLevel() - (e->pos().y() - grat_ll.y()) * yScale;
        MainWindow::GetStatusBar()->SetCursorPos(
                    Frequency(x).GetFreqString() + "  " +
                    Amplitude(y, s->RefLevel().Units()).GetString());

    } else {
        MainWindow::GetStatusBar()->SetCursorPos("");
    }
}

void TGPlot::paintEvent(QPaintEvent *)
{
    makeCurrent();

    glQClearColor(GetSession()->colors.background);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);

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

    BuildGraticule();
    DrawGraticule();
    DrawTraces();

    glDisable(GL_DEPTH_TEST);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    RenderMarkers();

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

// Build/reload just the graticule border
void TGPlot::BuildGraticule()
{
    float border[20] =
    {
        0.0, 0.0, 1.0, 0.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        0.0, 1.0, 0.0, 0.0,

        0.0, 0.0, 1.0, 0.0
    };

    double ref = GetSession()->sweep_settings->RefLevel().Val();
    double div = GetSession()->sweep_settings->Div();
    double grat_bottom = ref - 10.0*div;

    float pos = (0.0 - grat_bottom) / (ref - grat_bottom);
    if(pos < 0.0) pos = 0.0;
    if(pos > 1.0) pos = 1.0;

    border[17] = border[19] = pos;

    glBindBuffer(GL_ARRAY_BUFFER, gratBorderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(border),
                 border, GL_DYNAMIC_DRAW);
    borderGratPoints = 20;
}

void TGPlot::DrawTraces()
{
    TraceManager *manager = GetSession()->trace_manager;
    SweepSettings *settings = GetSession()->sweep_settings;

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
            normalize_trace(trace, traces[i], grat_sz,
                            settings->RefLevel(), settings->Div());
            DrawTrace(trace, traces[i], traceVBO);
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

void TGPlot::DrawTrace(const Trace *t, const GLVector &v, GLuint vbo)
{
    if(v.size() < 1) {
        return;
    }

    QColor c = t->Color();
    glColor3f(c.redF(), c.greenF(), c.blueF());

    // Put the trace in the vbo
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
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

void TGPlot::DrawGratText(QPainter &p)
{
    const SweepSettings *s = GetSession()->sweep_settings;
    TraceManager *tm = GetSession()->trace_manager;
    QVariant elapsed = time.restart();
    QString str;
    int textHeight = textFont.GetTextHeight();

    double div = s->RefLevel().IsLogScale() ? s->Div() : (s->RefLevel().Val() / 10.0);

    str = GetSession()->GetTitle();
    if(!str.isNull()) {
        p.setFont(QFont("Arial", 20));
        DrawString(p, str, width() / 2, height() - 22, CENTER_ALIGNED);
    }

    p.setFont(textFont.Font());

    TgCalState tgState = GetSession()->device->GetTgCalState();
    if(tgState == tgCalStateUncalibrated) {
        str.sprintf("Uncalibrated");
    } else if(tgState == tgCalStatePending) {
        str.sprintf("Calibration on next sweep");
    } else {
        str.sprintf("");
    }
    DrawString(p, str, grat_ll.x() + 5, grat_ll.y() + 5, LEFT_ALIGNED);

    str.sprintf("%d pts in %d ms", tm->GetTrace(0)->Length(), elapsed.toInt());
    DrawString(p, str, grat_ll.x()+grat_sz.x()-5,
               grat_ll.y()-textHeight*2, RIGHT_ALIGNED);
    DrawString(p, "Center " + s->Center().GetFreqString(),
               grat_ll.x() + grat_sz.x()/2, grat_ll.y()-textHeight, CENTER_ALIGNED);
    DrawString(p, "Span " + s->Span().GetFreqString(),
               grat_ll.x() + grat_sz.x()/2, grat_ll.y()-textHeight*2, CENTER_ALIGNED);
    DrawString(p, "Start " + (s->Start()).GetFreqString(),
               grat_ll.x()+5, grat_ll.y()-textHeight, LEFT_ALIGNED);
    DrawString(p, "Stop " + (s->Stop()).GetFreqString(),
               grat_ll.x()+grat_sz.x()-5, grat_ll.y()-textHeight, RIGHT_ALIGNED);
    str.sprintf("%.2f dB", s->RefLevel().Val());
    DrawString(p, "Ref " + str, grat_ll.x()+5, grat_ul.y()+textHeight, LEFT_ALIGNED);
    str.sprintf("Div %.1f", div);
    DrawString(p, str, grat_ul.x()+5, grat_ul.y()+2 , LEFT_ALIGNED);
    DrawString(p, "Step " + Frequency(tgStepSize).GetFreqString(),
               grat_ll.x() + grat_sz.x()/2, grat_ul.y()+textHeight, CENTER_ALIGNED);
    DrawString(p, "Atten --", grat_ll.x() + grat_sz.x()/2, grat_ul.y()+2, CENTER_ALIGNED);
    DrawString(p, "VBW --", grat_ul.x()+grat_sz.x()-5,
               grat_ul.y()+textHeight, RIGHT_ALIGNED);

    // y-axis labels
    p.setFont(divFont.Font());
    for(int i = 0; i <= 8; i += 2) {
        int x_pos = 58, y_pos = (grat_sz.y() / 10) * i + grat_ll.y() - 5;
        QString div_str;
        div_str.sprintf("%.2f", s->RefLevel() - (div*(10-i)));
        DrawString(p, div_str, x_pos, y_pos, RIGHT_ALIGNED);
    }

    p.setFont(textFont.Font());
    if(tm->GetLimitLine()->Active()) {
        QPoint limitTextLoc(grat_ul.x() + (grat_sz.x() * 0.5),
                            grat_ul.y() - (grat_sz.y() * 0.25));
        if(tm->GetLimitLine()->LimitsPassed()) {
            //glColor3f(0.0, 1.0, 0.0);
            p.setPen(QColor(0, 255, 0));
            DrawString(p, "Passed", limitTextLoc, CENTER_ALIGNED);
        } else {
            //glColor3f(1.0, 0.0, 0.0);
            p.setPen(QColor(255, 0, 0));
            DrawString(p, "Failed", limitTextLoc, CENTER_ALIGNED);
        }
    }

    // Amplitude high warning
    if(GetSession()->trace_manager->LastTraceAboveReference()) {
        glColor3f(1.0, 0.0, 0.0);
        p.setPen(QColor(255, 0, 0));
        DrawString(p, "*Warning* : Signal Level Higher Than Ref Level",
                   (grat_ul.x() + grat_sz.x()) / 2.0, grat_ul.y() - 22, CENTER_ALIGNED);
    }

    if(GetSession()->device->IsOpen()) {
        // Uncal text strings
        bool uncal = false;
        int uncal_x = grat_ul.x() + 5, uncal_y = grat_ul.y() - textHeight;
        //glColor3f(1.0, 0.0, 0.0);
        p.setPen(QColor(255, 0, 0));
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

void TGPlot::RenderMarkers()
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

    tm->SolveMarkers(s);

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

        // Does not have to be in view to draw the delta values
        if(m->DeltaActive()) {
            glQColor(GetSession()->colors.text);
//            DrawString("Mkr " + QVariant(i+1).toString() + " Delta: " + m->DeltaText(),
//                       textFont, QPoint(x_print, y_print), RIGHT_ALIGNED);
            AddTextToRender("Mkr " + QVariant(i+1).toString() + " Delta: " + m->DeltaText(),
                            QPoint(x_print, y_print), RIGHT_ALIGNED, textFont.Font(),
                            GetSession()->colors.text);
            y_print -= 20;
        } else if(m->Active()) {
            QString markerReadout = m->Freq().GetFreqString() + ", " +
                    QString().sprintf("%.2f dB", m->Amp().Val());
            glQColor(GetSession()->colors.text);
//            DrawString("Mkr " + QVariant(i+1).toString() + ": " + markerReadout,
//                       textFont, QPoint(x_print, y_print), RIGHT_ALIGNED);
            AddTextToRender("Mkr " + QVariant(i+1).toString() + ": " + markerReadout,
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

void TGPlot::DrawMarker(int x, int y, int num)
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
//    DrawString(str, divFont,
//               QPoint(x, y + 10), CENTER_ALIGNED);
    AddTextToRender(str, QPoint(x, y+10), CENTER_ALIGNED,
                    divFont.Font(), GetSession()->colors.markerText);
}

void TGPlot::DrawDeltaMarker(int x, int y, int num)
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
    //DrawString(str, divFont, QPoint(x, y+11), CENTER_ALIGNED);
    AddTextToRender(str, QPoint(x, y+11), CENTER_ALIGNED,
                    divFont.Font(), GetSession()->colors.markerText);
}
