#include "harmonics_spectrum.h"

static const float harmonicBorderPoints[] =
{
    0.0, 0.0, 1.0, 0.0,
    1.0, 0.0, 1.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    0.0, 1.0, 0.0, 0.0,

    0.2, 0.0, 0.2, 1.0,
    0.4, 0.0, 0.4, 1.0,
    0.6, 0.0, 0.6, 1.0,
    0.8, 0.0, 0.8, 1.0
};
static const size_t harmonicBorderPointCount =
        sizeof(harmonicBorderPoints) / sizeof(float);

HarmonicsSpectrumPlot::HarmonicsSpectrumPlot(Session *sPtr, QWidget *parent) :
    GLSubView(sPtr, parent),
    textFont(12),
    divFont(12)
{
    for(int i = 0; i < 5; i++) {
        harmonics[i].SetType(NORMAL);
    }

    makeCurrent();

    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    context()->format().setDoubleBuffer(true);

    glBindBuffer(GL_ARRAY_BUFFER, gratBorderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(harmonicBorderPoints),
                 harmonicBorderPoints, GL_DYNAMIC_DRAW);
    borderGratPoints = harmonicBorderPointCount;

    glGenBuffers(5, traceBufferObjects);

    doneCurrent();
}

HarmonicsSpectrumPlot::~HarmonicsSpectrumPlot()
{
    makeCurrent();
    glDeleteBuffers(5, traceBufferObjects);
    doneCurrent();
}

void HarmonicsSpectrumPlot::resizeEvent(QResizeEvent *)
{
    SetGraticuleDimensions(QPoint(60, 50),
                           QPoint(width() - 80, height() - 100));
}

void HarmonicsSpectrumPlot::paintEvent(QPaintEvent *)
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

    for(int i = 0; i < 5; i++) {
        double freq = 0.0;
        harmonics[i].GetSignalPeak(&freq, &markerReadings[i]);

        markers[i].Place(freq);
        markers[i].UpdateMarker(&harmonics[i], GetSession()->sweep_settings);
    }

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

void HarmonicsSpectrumPlot::DrawTrace(const Trace *t, const GLVector &v, GLuint vbo)
{
    if(v.size() < 1) {
        return;
    }

    glColor3f(0.0, 0.0, 0.0);

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

void HarmonicsSpectrumPlot::DrawTraces()
{
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

    for(int i = 0; i < 5; i++) {
        //traceLock[i].lock();
        if(harmonics[i].Length() <= 0) continue;
        normalize_trace(&harmonics[i], normalizedTraces[i],
                        QPoint(grat_sz.x()/5, grat_sz.y()));
        glViewport(grat_ll.x() + (grat_sz.x() / 5) * i,
                   grat_ll.y(), grat_sz.x() / 5, grat_sz.y());
        DrawTrace(&harmonics[i], normalizedTraces[i], traceBufferObjects[i]);
        //traceLock[i].unlock();
    }

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

void HarmonicsSpectrumPlot::DrawGratText(QPainter &p)
{
    QString str;
    const SweepSettings *s = GetSession()->sweep_settings;
    int textHeight = textFont.GetTextHeight();

    double div = s->RefLevel().IsLogScale() ? s->Div() : (s->RefLevel().Val() / 10.0);

    p.setFont(QFont("Arial", 20));
    str = GetSession()->GetTitle();
    if(!str.isNull()) {
        DrawString(p, str, width() / 2, height() - 22, CENTER_ALIGNED);
    }

    p.setFont(textFont.Font());
    DrawString(p, "Center " + s->Center().GetFreqString(),
               grat_ll.x() + grat_sz.x()/10, grat_ll.y() - textHeight, CENTER_ALIGNED);
    str.sprintf("%.2f %s", markerReadings[0], s->RefLevel().IsLogScale() ? "dBm" : "mV");
    DrawString(p, str, grat_ll.x() + grat_sz.x()/10,
               grat_ll.y() - textHeight*2, CENTER_ALIGNED);
    for(int i = 1; i < 5; i++) {
        if(i == 1) str.sprintf("2nd Harmonic");
        else if(i == 2) str.sprintf("3rd Harmonic");
        else if(i == 3) str.sprintf("4th Harmonic");
        else if(i == 4) str.sprintf("5th Harmonic");
        int xPos = grat_ll.x() + grat_sz.x()/10 + (i*grat_sz.x()/5);
        DrawString(p, str, xPos, grat_ll.y() - textHeight, CENTER_ALIGNED);
        str.sprintf("%.2f %s", markerReadings[i] - markerReadings[0],
                s->RefLevel().IsLogScale() ? "dBc" : "mV");
        DrawString(p, str, xPos, grat_ll.y() - textHeight*2, CENTER_ALIGNED);
    }

    DrawString(p, "Ref " + s->RefLevel().GetString(),
               grat_ll.x()+5, grat_ul.y()+textHeight, LEFT_ALIGNED);
    str.sprintf("Div %.1f", div);
    DrawString(p, str, grat_ul.x()+5, grat_ul.y()+2 , LEFT_ALIGNED);
    DrawString(p, "RBW " + s->RBW().GetFreqString(),
               grat_ll.x() + grat_sz.x()/2, grat_ul.y()+textHeight, CENTER_ALIGNED);
    s->GetAttenString(str);
    DrawString(p, str, grat_ll.x() + grat_sz.x()/2, grat_ul.y()+2, CENTER_ALIGNED);
    DrawString(p, "VBW " + s->VBW().GetFreqString(),
               grat_ul.x()+grat_sz.x()-5, grat_ul.y()+textHeight, RIGHT_ALIGNED);

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

void HarmonicsSpectrumPlot::DrawMarker(int x, int y, int num)
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
//    DrawString(str, divFont,
//               QPoint(x, y + 10), CENTER_ALIGNED);
}

void HarmonicsSpectrumPlot::DrawMarkers()
{
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, width(), height());
    //glViewport(grat_ll.x(), grat_ll.y(), grat_sz.x(), grat_sz.y());

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    //glOrtho(0, grat_sz.x(), 0, grat_sz.y(), -1, 1);
    glOrtho(0, width(), 0, height(), -1, 1);

    // Nice lines, doesn't smooth quads
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glLineWidth(1.0);

    for(int i = 0; i < 5; i++) {
        Marker *m = &markers[i];
        DrawMarker(grat_ll.x() + m->xRatio()*grat_sz.x()/5 + i*grat_sz.x()/5,
                   grat_ll.y() + m->yRatio()*grat_sz.y(),
                   i + 1);
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
