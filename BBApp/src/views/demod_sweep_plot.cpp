#include "demod_sweep_plot.h"

#include <QLayout>
#include <QMouseEvent>

#include <limits>

DemodSweepPlot::DemodSweepPlot(Session *session, QWidget *parent) :
    GLSubView(session, parent),
    textFont(14),
    divFont(12),
    demodType(DemodTypeAM),
    markerOn(false),
    deltaOn(false)
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

DemodSweepPlot::~DemodSweepPlot()
{
    makeCurrent();
    glDeleteBuffers(1, &traceVBO);
    doneCurrent();
}

void DemodSweepPlot::resizeEvent(QResizeEvent *)
{
    SetGraticuleDimensions(QPoint(60, 50),
                           QPoint(width() - 80, height() - 100));
}

void DemodSweepPlot::mousePressEvent(QMouseEvent *e)
{
    QRect area(grat_ll.x(), grat_ll.y(), grat_sz.x(), grat_sz.y());
    if(area.contains(e->pos())) {
        markerOn = true;
        markerPos.setX((double)(e->pos().x() - grat_ul.x()) / grat_sz.x());
    }
    update();

    QGLWidget::mousePressEvent(e);
}

void DemodSweepPlot::paintEvent(QPaintEvent *)
{
    makeCurrent();

    if(GetSession()->demod_settings->MAEnabled()) {
        grat_sz.setX(width() / 2 - 80);
    } else {
        grat_sz.setX(width() - 80);
    }

    glQClearColor(GetSession()->colors.background);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);

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
                           QPoint(grat_sz.x(), height() - textHeight*4));

    glViewport(0, 0, width(), height());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    DrawGraticule();
    DemodAndDraw();

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
    DrawPlotText(p);

    p.end();
    glPopAttrib();

    doneCurrent();
    swapBuffers();
}

void DemodSweepPlot::DemodAndDraw()
{
    const DemodSettings *ds = GetSession()->demod_settings;
    const IQSweep &sweep = GetSession()->iq_capture;
    if(sweep.sweepLen <= 4) return;
    if(sweep.iq.size() <= 1) return;

    trace.clear();

    double ref, botRef;

    if(demodType == DemodTypeAM) {
        if(ds->InputPower().IsLogScale()) {
            ref = ds->InputPower().ConvertToUnits(DBM);
            botRef = ref - 100.0;
            for(int i = 0; i < sweep.amWaveform.size(); i++) {
                trace.push_back(i);
                trace.push_back(10.0 * log10(sweep.amWaveform[i]));
            }
        } else {
            ref = ds->InputPower();
            botRef = 0.0;
            for(int i = 0; i < sweep.amWaveform.size(); i++) {
                trace.push_back(i);
                trace.push_back(sqrt((double)sweep.amWaveform[i] * 50000.0));
            }
        }

    } else if(demodType == DemodTypeFM) {
        ref = sweep.descriptor.sampleRate / 2.0;
        botRef = -ref;

        for(int i = 0; i < sweep.fmWaveform.size(); i++) {
            trace.push_back(i);
            trace.push_back(sweep.fmWaveform[i]);
        }

    } else if(demodType == DemodTypePM) {
        ref = BB_PI;
        botRef = -BB_PI;

        for(int i = 0; i < sweep.pmWaveform.size(); i++) {
            trace.push_back(i);
            trace.push_back(sweep.pmWaveform[i]);
        }
    }

    glViewport(grat_ll.x(), grat_ll.y(), grat_sz.x(), grat_sz.y());
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, trace.size()/2 - 1, botRef, ref, -1, 1);

    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glLineWidth(GetSession()->prefs.trace_width);

    qglColor(QColor(255, 0, 0));
    DrawTrace(trace);

    if(GetSession()->demod_settings->TrigType() != TriggerTypeNone) {
        // Draw trigger position line, vertical line
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, sweep.sweepLen-1, 0.0, 1.0, -1.0, 1.0);
        glColor4f(0.5, 0.5, 0.5, 0.5);
        glLineWidth(2.0);
        glBegin(GL_LINE_STRIP);
        //int xPos = (double)sweep.preTrigger / sweep.sweepLen * grat_sz.x();
        glVertex2f(sweep.preTrigger, 0);
        glVertex2f(sweep.preTrigger, 1.0);
        glEnd();
    }

    // Disable nice lines
    glLineWidth(1.0);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void DemodSweepPlot::DrawPlotText(QPainter &p)
{
    int textHeight = textFont.GetTextHeight();

    const DemodSettings *ds = GetSession()->demod_settings;
    const IQSweep &sweep = GetSession()->iq_capture;
    QString str;

    DrawString(p, "Center " + ds->CenterFreq().GetFreqString(),
               grat_ul.x() + grat_sz.x()/2, grat_ll.y() - textHeight, CENTER_ALIGNED);
    str.sprintf("%f ms per div", ds->SweepTime() * 100.0);
    DrawString(p, str, grat_ll.x() + grat_sz.x() - 5, grat_ul.y() + 2, RIGHT_ALIGNED);
    str.sprintf("%d pts", GetSession()->iq_capture.sweepLen);
    DrawString(p, str, grat_ll.x() + grat_sz.x() - 5, grat_ll.y() - textHeight, RIGHT_ALIGNED);

    if(demodType == DemodTypeAM) {
        DrawString(p, "Ref " + ds->InputPower().GetString(),
                   QPoint(grat_ul.x() + 5, grat_ul.y() + 2), LEFT_ALIGNED);
        double botVal, step;
        if(ds->InputPower().IsLogScale()) {
            botVal = ds->InputPower() - 100.0;
            step = 10.0;
        } else {
            botVal = 0;
            step = ds->InputPower() / 10.0;
        }
        p.setFont(divFont.Font());
        for(int i = 0; i <= 8; i += 2) {
            int x_pos = grat_ul.x() - 2, y_pos = (grat_sz.y() / 10) * i + grat_ll.y() - 5;
            str.sprintf("%.2f", botVal + step * i);
            DrawString(p, str, x_pos, y_pos, RIGHT_ALIGNED);
        }
    } else if(demodType == DemodTypeFM) {
        Frequency ref = sweep.descriptor.sampleRate / 2.0;
        double div = ref / 5.0;
        DrawString(p, "Ref " + ref.GetFreqString(),
                   QPoint(grat_ul.x() + 5, grat_ul.y() + 2), LEFT_ALIGNED);
        p.setFont(divFont.Font());
        for(int i = 1; i <= 9; i += 2) {
            int x_pos = grat_ul.x() - 2, y_pos = (grat_sz.y() / 10) * i + grat_ll.y() - 5;
            str.sprintf("%.2fM", (-ref + i * div) * 1.0e-6);
            DrawString(p, str, x_pos, y_pos, RIGHT_ALIGNED);
        }
    } else if(demodType == DemodTypePM) {
        DrawString(p, "Ref 3.14159 Deg",
                   QPoint(grat_ul.x() + 5, grat_ul.y() + 2), LEFT_ALIGNED);
        p.setFont(divFont.Font());
        for(int i = 1; i <= 9; i += 2) {
            int x_pos = grat_ul.x() - 2, y_pos = (grat_sz.y() / 10) * i + grat_ll.y() - 5;
            str.sprintf("%.2f", - 3.14159 + i * ((2 * BB_PI) / 10.0));
            DrawString(p, str, x_pos, y_pos, RIGHT_ALIGNED);
        }
    }

    p.setFont(textFont.Font());
    if(ds->TrigType() == TriggerTypeExternal || ds->TrigType() == TriggerTypeVideo) {
        if(GetSession()->iq_capture.triggered) {
            str.sprintf("Triggered");
        } else {
            str.sprintf("Armed");
        }
        DrawString(p, str, grat_ul.x() + grat_sz.x() / 2,
                   grat_ul.y() + 2, CENTER_ALIGNED);
    }

    // Uncal text strings
    bool uncal = false;
    int uncal_x = grat_ul.x() + 5, uncal_y = grat_ul.y() - 22;
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
        DrawString(p, "Uncal", grat_ul.x() - 5, grat_ul.y() - 22, RIGHT_ALIGNED);
    }

    if(GetSession()->demod_settings->MAEnabled()) {
        glQColor(GetSession()->colors.text);
        DrawModAnalysisReport(p);
    }
}

void DemodSweepPlot::DrawMarkers()
{
    if(!markerOn) return;

    const DemodSettings *ds = GetSession()->demod_settings;
    const IQSweep &sweep = GetSession()->iq_capture;

    QString str, delStr;
    double binSize = 1.0 / sweep.descriptor.sampleRate;

    markerIndex = (trace.size() / 2) * markerPos.x();
    markerPos.setX((double)markerIndex / ((trace.size()-1) / 2));
    str += QString().sprintf("%.2f ms : ", markerIndex * binSize * 1000.0);
    //str = QVariant(markerIndex * binSize * 1000.0).toString() + " ms : ";
    int index = markerIndex * 2 + 1;

    // Clamp to size, no out of bound indexing please
    if(index < 0) index = 1;
    if(index > trace.size()) index = trace.size();

    markerVal = trace[index];

    if(demodType == DemodTypeAM) {
        Amplitude markerAmp;
        if(ds->InputPower().IsLogScale()) {
            markerAmp = Amplitude(markerVal).ConvertToUnits(ds->InputPower().Units());
            float botRef = ds->InputPower() - 100.0; // Always div == 10
            markerPos.setY((markerAmp - botRef) / 100.0); // Always div == 10
        } else {
            markerAmp = Amplitude(markerVal, MV);
            float botRef = 0.0;
            markerPos.setY(markerAmp / ds->InputPower());
        }
        str += markerAmp.GetString();
    } else if(demodType == DemodTypeFM) {
        float botRef = -sweep.descriptor.sampleRate / 2.0;
        markerPos.setY((markerVal - botRef) / fabs(2.0 * botRef));
        str += Frequency(markerVal).GetFreqString();
    } else if(demodType == DemodTypePM) {
        float botRef = -BB_PI;
        markerPos.setY((markerVal - botRef) / BB_TWO_PI);
        str += QVariant(markerVal).toString();
    }

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
//    glOrtho(0, grat_sz.x(), 0, grat_sz.y(), -1, 1);
    glOrtho(0, width(), 0, height(), -1, 1);

    // Nice lines
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glLineWidth(1.0);

    glDisable(GL_DEPTH_TEST);
    DrawMarker(grat_ll.x() + markerPos.x() * grat_sz.x(),
               grat_ll.y() + markerPos.y() * grat_sz.y(), 1);

    if(deltaOn) {
        DrawDeltaMarker(grat_ll.x() + deltaPos.x() * grat_sz.x(),
                        grat_ll.y() + deltaPos.y() * grat_sz.y(), 1);
        double diff = markerVal - deltaVal;
        Time(0).GetString();
        //delStr.sprintf("Delta : %.2f ms, ", (markerIndex - deltaIndex) * binSize * 1000.0);
        delStr = QString("Delta : ") +
                getPreciseTimeString((markerIndex - deltaIndex) * binSize) +
                QString(", ");
//        delStr = "Delta : ";
//        delStr += QVariant((markerIndex - deltaIndex) * binSize * 1000.0).toString() + " ms, ";
        if(demodType == DemodTypeAM) {
            QString val;
            val.sprintf("%.2f %s", diff, ds->InputPower().IsLogScale() ? "dB" : "mV");
            delStr += val;
        } else if(demodType == DemodTypeFM) {
            delStr += Frequency(diff).GetFreqString();
        } else {
            QString val;
            val.sprintf("%.2f rad", diff);
            delStr += val;
        }
    }

    glQColor(GetSession()->colors.text);
    AddTextToRender(str, QPoint(grat_ll.x() + grat_sz.x() - 5, grat_ll.y() + grat_sz.y() - 22),
                    RIGHT_ALIGNED, textFont.Font(), GetSession()->colors.text);
    AddTextToRender(delStr, QPoint(grat_ll.x() + grat_sz.x() - 5, grat_ll.y() + grat_sz.y() - 42),
                    RIGHT_ALIGNED, textFont.Font(), GetSession()->colors.text);
//    DrawString(str, textFont, grat_sz.x() - 5, grat_sz.y() - 22, RIGHT_ALIGNED);
//    DrawString(delStr, textFont, grat_sz.x() - 5, grat_sz.y() - 42, RIGHT_ALIGNED);

    // Disable nice lines
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

void DemodSweepPlot::DrawTrace(const GLVector &v)
{
    if(v.size() < 2) {
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

void DemodSweepPlot::DrawMarker(int x, int y, int num)
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

void DemodSweepPlot::DrawDeltaMarker(int x, int y, int num)
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
//    DrawString(str, divFont,
//               QPoint(x, y+11), CENTER_ALIGNED);
}

void DemodSweepPlot::DrawModAnalysisReport(QPainter &p)
{
    const ModAnalysisReport stats = GetSession()->iq_capture.stats;

    QPoint textHeight(0, textFont.GetTextHeight());
    QPoint leftPos(width() / 2 + grat_ll.x(), grat_ul.y() - 20);
    QPoint rightPos(leftPos.x() + grat_sz.x() / 2, leftPos.y() - textHeight.y() * 3.5);
    QString str;

    DrawString(p, "AM/FM Modulation Analysis", leftPos, LEFT_ALIGNED);
    leftPos -= textHeight*2;
    DrawString(p, "RF Center " + Frequency(stats.rfCenter).GetFreqString(12, true), leftPos, LEFT_ALIGNED);
    leftPos -= textHeight*1.5;

    DrawString(p, "FM RMS: " + Frequency(stats.fmRMS).GetFreqString(), leftPos, LEFT_ALIGNED);
    leftPos -= textHeight;
    DrawString(p, "FM Peak+: " + Frequency(stats.fmPeakPlus).GetFreqString(), leftPos, LEFT_ALIGNED);
    leftPos -= textHeight;
    DrawString(p, "FM Peak-: " + Frequency(stats.fmPeakMinus).GetFreqString(), leftPos, LEFT_ALIGNED);
    leftPos -= textHeight;
    DrawString(p, "FM Rate: " + Frequency(stats.fmAudioFreq).GetFreqString(), leftPos, LEFT_ALIGNED);
    leftPos -= textHeight*2;

    str.sprintf("AM RMS %.3lf%%", stats.amRMS * 100.0);
    DrawString(p, str, rightPos, LEFT_ALIGNED);
    rightPos -= textHeight;
    str.sprintf("AM Peak+ %.3lf%%", stats.amPeakPlus * 100.0);
    DrawString(p, str, rightPos, LEFT_ALIGNED);
    rightPos -= textHeight;
    str.sprintf("AM Peak- %.3lf%%", stats.amPeakMinus * 100.0);
    DrawString(p, str, rightPos, LEFT_ALIGNED);
    rightPos -= textHeight;
    DrawString(p, "AM Rate " + Frequency(stats.amAudioFreq).GetFreqString(), rightPos, LEFT_ALIGNED);
    rightPos -= textHeight;

    if(demodType == DemodTypeAM) {
        str.sprintf("AM SINAD %.2f dB", stats.amSINAD);
        DrawString(p, str, leftPos, LEFT_ALIGNED);
        leftPos -= textHeight;
        str.sprintf("AM THD %.2f %%", stats.amTHD * 100.0);
        DrawString(p, str, leftPos, LEFT_ALIGNED);
    } else if(demodType == DemodTypeFM) {
        str.sprintf("FM SINAD %.2f dB", stats.fmSINAD);
        DrawString(p, str, leftPos, LEFT_ALIGNED);
        leftPos -= textHeight;
        str.sprintf("FM THD %.2f %%", stats.fmTHD * 100.0);
        DrawString(p, str, leftPos, LEFT_ALIGNED);
    }
}
