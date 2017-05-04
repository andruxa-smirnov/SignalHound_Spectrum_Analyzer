#include "trace_view.h"
#include "model/session.h"
#include "model/trace.h"
#include "lib/bb_lib.h"
#include "mainwindow.h"

#include <QToolTip>
#include <QMouseEvent>
#include <QPushButton>

#include <vector>

#define PERSIST_WIDTH 1280
#define PERSIST_HEIGHT 720


#pragma warning(disable:4305)
#pragma warning(disable:4267)

void glPerspective(float angle, float aRatio, float near_val, float far_val)
{
    float R = tan((angle * (BB_PI / 360.0f))) * near_val;
    float T = R * aRatio;

    glFrustum(-T, T, -R, R, near_val, far_val);
}

void glLookAt(float ex, float ey, float ez,
              float cx, float cy, float cz,
              float ux, float uy, float uz)
{
    float f[3];    // Forward Vector
    float UP[3];   // Up Vector
    float s[3];    // Side Vector
    float u[3];	   // Recomputed up
    float LA[16];  // Look At Matrix

    // Find Forward Vector
    f[0] = cx - ex;
    f[1] = cy - ey;
    f[2] = cz - ez ;
    normalize(f);
    // Normalize Up
    UP[0] = ux;
    UP[1] = uy;
    UP[2] = uz;
    normalize(UP);
    // Find s and u
    cross_product(s, f, UP);
    normalize(s);
    cross_product(u, s, f);

    // Build Look At Matrix
    LA[0] = s[0]; LA[1] = u[0]; LA[2] = -f[0]; LA[3] = 0;
    LA[4] = s[1]; LA[5] = u[1]; LA[6] = -f[1]; LA[7] = 0;
    LA[8] = s[2]; LA[9] = u[2]; LA[10] = -f[2]; LA[11] = 0;
    LA[12] = 0; LA[13] = 0; LA[14] = 0; LA[15] = 1;

    glMultMatrixf(LA);
    glTranslatef(-ex, -ey, -ez);
}

static float rho = 1, theta = -0.5 * BB_PI, phi = 0.4 * BB_PI;
static int mx, my;
static bool dragging = false;
static const float RPP = 0.01;

TraceView::TraceView(Session *session, QWidget *parent)
    : GLSubView(session, parent),
      persist_on(false),
      clear_persistence(false),
      waterfall_state(WaterfallOFF),
      textFont(12),
      divFont(12),
      hasOpenGL3(false),
      canDrawRealTimePersistence(false),
      realTimePersistOn(true),
      realTimeIntensity(25)
{
    setAutoBufferSwap(false);
    setMouseTracking(true);

    time.start();

    for(int i = 0; i < 11; i++) {
        graticule.push_back(0.0);
        graticule.push_back(0.1 * i);
        graticule.push_back(1.0);
        graticule.push_back(0.1 * i);
    }

    for(int i = 0; i < 11; i++) {
        graticule.push_back(0.1 * i);
        graticule.push_back(0.0);
        graticule.push_back(0.1 * i);
        graticule.push_back(1.0);
    }

    grat_border.push_back(0.0);
    grat_border.push_back(0.0);
    grat_border.push_back(1.0);
    grat_border.push_back(0.0);
    grat_border.push_back(1.0);
    grat_border.push_back(1.0);
    grat_border.push_back(0.0);
    grat_border.push_back(1.0);
    grat_border.push_back(0.0);
    grat_border.push_back(0.0);

    makeCurrent();
    initializeOpenGLFunctions();

    if(!hasOpenGLFeature(QOpenGLFunctions::Buffers)) {
        // Doesn't have support for Vertex buffers!
        // What to do?
    }

    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    context()->format().setDoubleBuffer(true);

    glGenBuffers(1, &traceVBO);
    glGenBuffers(1, &textureVBO);
    glGenBuffers(1, &gratVBO);
    glGenBuffers(1, &borderVBO);

    glBindBuffer(GL_ARRAY_BUFFER, gratVBO);
    glBufferData(GL_ARRAY_BUFFER, graticule.size()*sizeof(float),
                 &graticule[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
    glBufferData(GL_ARRAY_BUFFER, grat_border.size()*sizeof(float),
                 &grat_border[0], GL_STATIC_DRAW);

    // See if we can draw line persistence
    if(hasOpenGLFeature(QOpenGLFunctions::Framebuffers)
            && hasOpenGLFeature(QOpenGLFunctions::Shaders)) {
        persist_program = std::unique_ptr<GLProgram>(
                    new GLProgram(persist_vs, persist_fs));
        persist_program->Compile(this);
        InitPersistFBO();
        hasOpenGL3 = true;
    }

    // See if we can draw real-time persistence images
    if(hasOpenGLFeature(QOpenGLFunctions::Shaders)
            && hasOpenGLFeature(QOpenGLFunctions::NPOTTextures)) {
        realTimeShader = std::unique_ptr<GLProgram>(
                    new GLProgram(dot_persist_temp_vs, dot_persist_temp_fs));
        realTimeShader->Compile(this);
        realTimeScalar = glGetUniformLocation(realTimeShader->ProgramHandle(), "scale");
        canDrawRealTimePersistence = true;
    }

    waterfall_tex = get_texture_from_file(":/color_spectrogram.png");

    glGenTextures(1, &realTimeTexture);

    doneCurrent();

    swap_thread = new SwapThread(this);
    swap_thread->start();
    Sleep(25); // Wait for thread to start? Find better way.
}

TraceView::~TraceView()
{
    swap_thread->Stop();
    paintCondition.notify();
    swap_thread->wait();

    makeCurrent();
    glDeleteBuffers(1, &traceVBO);
    glDeleteBuffers(1, &textureVBO);
    glDeleteBuffers(1, &gratVBO);
    glDeleteBuffers(1, &borderVBO);

    glDeleteTextures(1, &waterfall_tex);
    glDeleteTextures(1, &realTimeTexture);

    doneCurrent();

    delete swap_thread;
    ClearWaterfall();

    Sleep(100); // Odd crashes on closing too quick? Still exist?
}

bool TraceView::InitPersistFBO()
{
    bool complete = false;

    // Render buffer start, build the tex we are writing to
    glGenTextures(1, &persist_tex);
    glBindTexture(GL_TEXTURE_2D, persist_tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap
    glTexImage2D(GL_TEXTURE_2D,	0, GL_RGBA,	PERSIST_WIDTH, PERSIST_HEIGHT,
                 0, GL_RGBA,	GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Generate our FBO depth buffer
    glGenRenderbuffers(1, &persist_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, persist_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                          PERSIST_WIDTH, PERSIST_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &persist_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, persist_fbo);

    // attach the texture to FBO color attachment point
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, persist_tex, 0);

    // attach the renderbuffer to depth attachment point
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, persist_depth);

    // check FBO status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        //pDoc->PutLog("FBO Not Complete\n");
        complete = false;
    } else {
        complete = true;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}

void TraceView::resizeEvent(QResizeEvent *)
{
    grat_ll.setX(60);
    grat_ul.setX(60);
    grat_sz.setX(width() - 80);
}

/*
 * In future, if currently drawing, queue up one draw? if possible
 */
void TraceView::paintEvent(QPaintEvent *)
{
    // Draw if possible, otherwise nothing?
    if(drawMutex.try_lock()) {
        paintCondition.notify();
        Paint();
        context()->moveToThread(swap_thread);
        drawMutex.unlock();
    }
}

// Place marker in simple case
void TraceView::mousePressEvent(QMouseEvent *e)
{
    if(PointInGrat(e->pos())) {
        // Make point relative to upper left of graticule
        int x_pos = e->pos().x() - grat_ul.x();

        if(x_pos < 0 || x_pos > grat_sz.x()) {
            return;
        }

        GetSession()->trace_manager->PlaceMarkerPercent((double)x_pos / grat_sz.x());

    } else if (waterfall_state == Waterfall3D) {
        dragging = true;
        mx = e->pos().x();
        my = e->pos().y();
    }

    QGLWidget::mousePressEvent(e);
}

void TraceView::mouseReleaseEvent(QMouseEvent *)
{
    dragging = false;
}

void TraceView::mouseMoveEvent(QMouseEvent *e)
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

        y = s->RefLevel() - (e->pos().y() - (height() - grat_ul.y())) * yScale;
        MainWindow::GetStatusBar()->SetCursorPos(
                    Frequency(x).GetFreqString() + "  " +
                    Amplitude(y, s->RefLevel().Units()).GetString());

    } else {
        MainWindow::GetStatusBar()->SetCursorPos("");
    }

    if(dragging) {
        // This math affects our spherical coords for viewing 3D waterfall
        int dx = e->pos().x() - mx, dy = e->pos().y() - my;
        theta -= dx * RPP;
        phi -= dy * RPP;
        if(phi < 0.1 * BB_PI) phi = 0.1 * BB_PI;
        if(phi > 0.5 * BB_PI) phi = 0.5 * BB_PI;
        if(theta < -0.75 * BB_PI) theta = -0.75 * BB_PI;
        if(theta > -0.25 * BB_PI) theta = -0.25 * BB_PI;
        mx = e->pos().x();
        my = e->pos().y();
        update();
    }

    QGLWidget::mouseMoveEvent(e);
}

void TraceView::wheelEvent(QWheelEvent *e)
{
    if(e->delta() < 0) rho += 0.1;
    if(e->delta() > 0) rho -= 0.1;
    if(rho < 0.5) rho = 0.5;
    if(rho > 4.0) rho = 4.0;
    update();

    QGLWidget::wheelEvent(e);
}

/*
 * Main Paint Routine
 */
void TraceView::Paint()
{
    // Before we paint, ensure the glcontext belongs to the main thread
    if(context()->contextHandle()->thread() != QApplication::instance()->thread()) {
        return;
    }

    makeCurrent();

    glQClearColor(GetSession()->colors.background, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Must be called for textures to be drawn properly
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    if(grat_sz.x() >= 600) {
        textFont = GLFont(14);
    } else if(grat_sz.x() <= 350) {
        textFont = GLFont(8);
    } else {
        int mod = (600 - grat_sz.x()) / 50;
        textFont = GLFont(13 - mod);
    }

    int textHeight = textFont.GetTextHeight() + 2;
    grat_ll.setY(textHeight * 2);
    grat_ul.setY(height() - (textHeight*2));
    grat_sz.setY(height() - (textHeight*4));

    // Draw nothing but background color if the graticule has a
    //   negative width or height
    if(width() <= 80 || grat_sz.y() <= 0) {
        return;
    }

    // Calculate dimensions for the presence of the title bar or waterfall
    if(!GetSession()->GetTitle().isNull()) {
        grat_ul -= QPoint(0, 20);
        grat_sz -= QPoint(0, 20);
    }
    if(waterfall_state != WaterfallOFF) {
        grat_ul -= QPoint(0, height() / 2);
        grat_sz -= QPoint(0, height() / 2);
    }

    glViewport(0, 0, width(), height());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width(), 0, height(), -1, 1);

    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);

    RenderGraticule(); // Graticule always shown

    bool drawGratText = true;

    if(!GetSession()->device->IsOpen() && !GetSession()->isInPlaybackMode) {
//        glQColor(GetSession()->colors.text);
//        DrawString(tr("No Device Connected"), textFont,
//                   QPoint(grat_ul.x(), grat_ul.y() + 2), LEFT_ALIGNED);
        AddTextToRender("No Device Connected", QPoint(grat_ul.x(), grat_ul.y() + 2),
                        LEFT_ALIGNED, textFont.Font(), GetSession()->colors.text);
        drawGratText = false;

    } else if(GetSession()->sweep_settings->Mode() == BB_IDLE &&
              !GetSession()->isInPlaybackMode) {
//        glQColor(GetSession()->colors.text);
//        DrawString(tr("Device Idle"), textFont,
//                   QPoint(grat_ul.x(), grat_ul.y() + 2), LEFT_ALIGNED);
        AddTextToRender("Device Idle", QPoint(grat_ul.x(), grat_ul.y() + 2),
                        LEFT_ALIGNED, textFont.Font(), GetSession()->colors.text);
        drawGratText = false;
    } else {

        RenderTraces();
        if(waterfall_state != WaterfallOFF) {
            DrawWaterfall();
        }

        glDisable(GL_DEPTH_TEST);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        RenderMarkers();
        RenderChannelPower();
        RenderOccupiedBandwidth();
    }

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(0.0, height());
    p.setPen(QPen(GetSession()->colors.text));
    p.setFont(textFont.Font());

    DrawTextQueue(p);
    if(drawGratText) {
        RenderGratText(p);
    }

    p.end();
    glPopAttrib();
    doneCurrent();

    // historical
//    glPushAttrib(GL_ALL_ATTRIB_BITS);
//    QPainter p(this);
//    p.setBackgroundMode(Qt::TransparentMode);
//    p.setRenderHint(QPainter::TextAntialiasing);
//    p.setRenderHint(QPainter::Antialiasing);
//    p.setFont(textFont.Font());
//    for(int i = 0; i < 100; i++) {
//        QString s;
//        s.sprintf("Hello World %f", i * 1.12345);
//        p.drawText(200, i*5, s);
//    }
//    p.end();
//    glPopAttrib();

//    doneCurrent();
}

void TraceView::RenderGraticule()
{
    // Model view for graticule
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(grat_ll.x(), grat_ll.y(), 0);
    glScalef(grat_sz.x(), grat_sz.y(), 1.0);

    // Projection for graticule
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, size().width(), 0, size().height(), -1, 1);

    glLineWidth(GetSession()->prefs.graticule_width);
    glQColor(GetSession()->colors.graticule);

    // Draw inner grat
    if(GetSession()->prefs.graticule_stipple) {
        glLineStipple(1, 0x8888);
        glEnable(GL_LINE_STIPPLE);
    }

    glBindBuffer(GL_ARRAY_BUFFER, gratVBO);
    glVertexPointer(2, GL_FLOAT, 0, INDEX_OFFSET(0));
    glDrawArrays(GL_LINES, 0, graticule.size()/2);

    if(GetSession()->prefs.graticule_stipple) {
        glDisable(GL_LINE_STIPPLE);
    }

    // Border
    glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
    glVertexPointer(2, GL_FLOAT, 0, INDEX_OFFSET(0));
    glDrawArrays(GL_LINE_STRIP, 0, grat_border.size()/2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glLineWidth(1.0);
}

void TraceView::RenderGratText(QPainter &p)
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
    if(tm->GetLimitLine()->Active()) {
        QPoint limitTextLoc(grat_ul.x() + (grat_sz.x() * 0.5),
                            grat_ul.y() - (grat_sz.y() * 0.25));
        if(tm->GetLimitLine()->LimitsPassed()) {
            //glColor3f(0.0, 1.0, 0.0);
            p.setPen(QPen(QColor(0, 255, 0)));
            DrawString(p, "Passed", limitTextLoc, CENTER_ALIGNED);
        } else {
            //glColor3f(1.0, 0.0, 0.0);
            p.setPen(QPen(QColor(255, 0, 0)));
            DrawString(p, "Failed", limitTextLoc, CENTER_ALIGNED);
        }
    }

    // Amplitude high warning
    if(GetSession()->trace_manager->LastTraceAboveReference()) {
        glColor3f(1.0, 0.0, 0.0);
        p.setPen(QPen(QColor(255, 0, 0)));
        DrawString(p, "*Warning* : Signal Level Higher Than Ref Level",
                   (grat_ul.x() + grat_sz.x()) / 2.0, grat_ul.y() - 22, CENTER_ALIGNED);
    }

    if(GetSession()->device->IsOpen()) {
        // Uncal text strings
        bool uncal = false;
        int uncal_x = grat_ul.x() + 5, uncal_y = grat_ul.y() - textHeight;
        //glColor3f(1.0, 0.0, 0.0);
        p.setPen(QPen(QColor(255, 0, 0)));
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
        if(GetSession()->trace_manager->GetPathLossTable().Active()) {
            DrawString(p, "PLT", uncal_x, uncal_y, LEFT_ALIGNED);
            uncal_y -= textHeight;
        }
        if(uncal) {
            DrawString(p, "Uncal", grat_ul.x() - 5, grat_ul.y() + 2, RIGHT_ALIGNED);
        }
    }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void TraceView::RenderTraces()
{
    TraceManager *manager = GetSession()->trace_manager;

    // Un-buffer persist/waterfall data
    if(persist_on || (waterfall_state != WaterfallOFF)) {
        GLVector *v_ptr = nullptr;
        while(v_ptr = manager->trace_buffer.Back()) {
            if(persist_on) {
                AddToPersistence(*v_ptr);
            }
            if(waterfall_state != WaterfallOFF) {
                AddToWaterfall(*v_ptr);
            }
            manager->trace_buffer.IncrementBack();
        }
    }

    if(GetSession()->sweep_settings->IsRealTime()) {
        if(realTimePersistOn && canDrawRealTimePersistence) {
            DrawRealTimeFrame();
        }
    } else if(persist_on && HasOpenGL3()) {
        DrawPersistence();
        return;
    }

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
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glLineWidth(GetSession()->prefs.trace_width);

    manager->Lock();

    // Loop through each trace
    for(int i = 0; i < TRACE_COUNT; i++) {
        // If Trace is active, normalize and draw it
        const Trace *trace = manager->GetTrace(i);

        if(trace->Active()) {
            normalize_trace(trace, traces[i], grat_sz);
            DrawTrace(trace, traces[i]);
        }
    }

    manager->Unlock();

    if(manager->GetLimitLine()->Active()) {
        const SweepSettings *ss = GetSession()->sweep_settings;
        normalize_trace(&manager->GetLimitLine()->store,
                        traces[0],
                        grat_sz,
                        ss->RefLevel(),
                        ss->Div());
        DrawLimitLines(&manager->GetLimitLine()->store, traces[0]);
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

void TraceView::DrawTrace(const Trace *t, const GLVector &v)
{
    if(v.size() < 1) {
        return;
    }

    QColor c = t->Color();
    glColor3f(c.redF(), c.greenF(), c.blueF());

    // Put the trace in the vbo
    glBindBuffer(GL_ARRAY_BUFFER, traceVBO);
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

void TraceView::RenderMarkers()
{
    SweepSettings *s = GetSession()->sweep_settings;
    TraceManager *tm = GetSession()->trace_manager;

    // Viewport on grat, full pixel scale
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, width(), height());

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
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
            AddTextToRender("Mkr " + QVariant(i+1).toString() + " Delta: " + m->DeltaText(),
                            QPoint(x_print, y_print), RIGHT_ALIGNED, textFont.Font(),
                            GetSession()->colors.text);
            y_print -= 20;
        } else if(m->Active()) {
            AddTextToRender("Mkr " + QVariant(i+1).toString() + ": " + m->Text(),
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

void TraceView::DrawMarker(int x, int y, int num)
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

void TraceView::DrawOCBWMarker(int x, int y, bool left)
{
    glLineWidth(2.0);

    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_POLYGON);
    glVertex2f(x, y - 5);
    glVertex2f(x + 5, y);
    glVertex2f(x, y + 5);
    glVertex2f(x - 5, y );
    glEnd();

    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(x, y - 5);
    glVertex2f(x + 5, y);
    glVertex2f(x, y + 5);
    glVertex2f(x - 5, y );
    glVertex2f(x, y - 5);
    glEnd();

    glBegin(GL_LINES);
    if(left) {
        glVertex2f(x - 20, y);
        glVertex2f(x - 30, y);
        glVertex2f(x - 20, y);
        glVertex2f(x - 25, y + 5);
        glVertex2f(x - 20, y);
        glVertex2f(x - 25, y - 5);
    } else {
        glVertex2f(x + 20, y);
        glVertex2f(x + 30, y);
        glVertex2f(x + 20, y);
        glVertex2f(x + 25, y + 5);
        glVertex2f(x + 20, y);
        glVertex2f(x + 25, y - 5);
    }
    glEnd();

    glLineWidth(1.0);
}

void TraceView::DrawDeltaMarker(int x, int y, int num)
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


void TraceView::RenderChannelPower()
{
    const ChannelPower *cp = GetSession()->trace_manager->GetChannelPowerInfo();
    if(!cp->IsEnabled()) return;

    double start = GetSession()->sweep_settings->Start();
    double stop = GetSession()->sweep_settings->Stop();
    double span = stop - start;
    if(span == 0.0) return;

    AmpUnits printUnits = (GetSession()->sweep_settings->RefLevel().Units() == MV) ?
                MV : DBM;

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(grat_ll.x(), grat_ll.y(), grat_sz.x(), grat_sz.y());
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, grat_sz.x(), 0, grat_sz.y(), -1, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(int i = 0; i < 3; i++) {
        if(!cp->IsChannelInView(i)) continue;

        double x1 = (cp->GetChannelStart(i) - start) / span,
                x2 = (cp->GetChannelStop(i) - start) / span;
        double xCen = (x1 + x2) / 2.0;

        if(i != 1) { // Push channel text out 100 px?
            int realCenter = grat_sz.x() / 2;
            // Get largest possible width with extra space
            int textWidth = textFont.GetTextWidth(" -12.345 dBm");
            if(abs(xCen*grat_sz.x() - realCenter) < textWidth) {
                xCen = double(realCenter + (i-1)*textWidth) / grat_sz.x();
            }
        }

        glColor4f(0.5, 0.5, 0.5, 0.4);
        glBegin(GL_QUADS);
        glVertex2f(x1 * grat_sz.x(), 0.0);
        glVertex2f(x2 * grat_sz.x(), 0.0);
        glVertex2f(x2 * grat_sz.x(), grat_sz.y());//1.0);
        glVertex2f(x1 * grat_sz.x(), grat_sz.y());//1.0);
        glEnd();

        // Draw channel power text
        glQColor(GetSession()->colors.text);
        QString cp_string;
        Amplitude power(cp->GetChannelPower(i), (printUnits == MV) ? MV : DBM);
//        DrawString(power.ConvertToUnits(printUnits).GetString(),
//                   textFont, xCen * grat_sz.x(), textFont.GetTextHeight()*3 + 2, CENTER_ALIGNED);
        AddTextToRender(power.ConvertToUnits(printUnits).GetString(),
                        QPoint(xCen * grat_sz.x() + grat_ll.x(),
                               textFont.GetTextHeight()*3 + 2 + grat_ll.y()),
                        CENTER_ALIGNED, textFont.Font(), GetSession()->colors.text);

        // Draw adjacent channel power text
        if(i == 0 || i == 2) {
            cp_string.sprintf("%.2f %s", cp->GetChannelPower(i) - cp->GetChannelPower(1),
                              (printUnits == MV) ? "mV" : "dBc");
//            DrawString(cp_string, textFont, xCen * grat_sz.x(),
//                       textFont.GetTextHeight()*2 + 2, CENTER_ALIGNED);
            AddTextToRender(cp_string, QPoint(xCen * grat_sz.x() + grat_ll.x(),
                                              textFont.GetTextHeight()*2 + 2 + grat_ll.y()),
                            CENTER_ALIGNED, textFont.Font(), GetSession()->colors.text);
        }
    }

    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

void TraceView::RenderOccupiedBandwidth()
{
    const TraceManager *tm = GetSession()->trace_manager;
    const OccupiedBandwidthInfo &info = tm->GetOccupiedBandwidthInfo();

    if(!info.enabled) return;

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(grat_ll.x(), grat_ll.y(), grat_sz.x(), grat_sz.y());
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, grat_sz.x(), 0, grat_sz.y(), -1, 1);

    DrawBackdrop(QPoint(info.leftMarker.xRatio() * grat_sz.x() - 35,
                        info.leftMarker.yRatio() * grat_sz.y() - 10), QPoint(45, 20));
    DrawOCBWMarker(info.leftMarker.xRatio() * grat_sz.x(),
                   info.leftMarker.yRatio() * grat_sz.y(), true);

    DrawBackdrop(QPoint(info.rightMarker.xRatio() * grat_sz.x() - 10,
                        info.rightMarker.yRatio() * grat_sz.y() - 10), QPoint(45, 20));
    DrawOCBWMarker(info.rightMarker.xRatio() * grat_sz.x(),
                   info.rightMarker.yRatio() * grat_sz.y(), false);

    DrawBackdrop(QPoint(1, 0), QPoint(400, textFont.GetTextHeight() * 2));
//    glColor3f(0.0, 0.0, 0.0);
//    DrawString("Occupied Bandwidth " + info.bandwidth.GetFreqString(),
//               textFont, 5, textFont.GetTextHeight(), LEFT_ALIGNED);
    AddTextToRender("Occupied Bandwidth " + info.bandwidth.GetFreqString(),
                    QPoint(5 + grat_ll.x(), textFont.GetTextHeight() + grat_ll.y()),
                    LEFT_ALIGNED, textFont.Font(), GetSession()->colors.text);
//    DrawString("Total Power " + info.totalPower.GetString(),
//               textFont, 5, 2, LEFT_ALIGNED);
    AddTextToRender("Total Power " + info.totalPower.GetString(),
                    QPoint(5 + grat_ll.x(), 2 + grat_ll.y()), LEFT_ALIGNED,
                    textFont.Font(), GetSession()->colors.text);


    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

void TraceView::DrawPersistence()
{
    // Draw a single quad over our grat
    glUseProgram(persist_program->ProgramHandle());
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, persist_tex);
    glGenerateMipmap(GL_TEXTURE_2D);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(grat_ll.x(), grat_ll.y(), 0.0);
    glScalef(grat_sz.x(), grat_sz.y(), 1.0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(0,0);
    glTexCoord2f(0,1); glVertex2f(0,1);
    glTexCoord2f(1,1); glVertex2f(1,1);
    glTexCoord2f(1,0); glVertex2f(1,0);
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    glUseProgram(0);
}

void TraceView::DrawRealTimeFrame()
{
    RealTimeFrame &frame = GetSession()->trace_manager->realTimeFrame;

    if(frame.dim.width() <= 0) return;
    if(frame.dim.height() <= 0) return;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, realTimeTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 frame.dim.width(),
                 256,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 &frame.rgbFrame[0]);

    // Draw a single quad over our grat
    glUseProgram(realTimeShader->ProgramHandle());
    glUniform1f(realTimeScalar, 100.0 / (100 - realTimeIntensity));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(grat_ll.x(), grat_ll.y(), 0.0);
    glScalef(grat_sz.x(), grat_sz.y(), 1.0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(0,0);
    glTexCoord2f(0,1); glVertex2f(0,1);
    glTexCoord2f(1,1); glVertex2f(1,1);
    glTexCoord2f(1,0); glVertex2f(1,0);
    glEnd();

    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    glUseProgram(0);
}

void TraceView::DrawLimitLines(const Trace *limitTrace, const GLVector &v)
{
    if(limitTrace->Length() < 1) return;

    glLineWidth(3.0);
    glQColor(GetSession()->colors.limitLines);

    glBindBuffer(GL_ARRAY_BUFFER, traceVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float),
                 &v[0], GL_DYNAMIC_DRAW);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Draw max
    glVertexPointer(2, GL_FLOAT, 4*sizeof(float), (GLvoid*)0);
    glDrawArrays(GL_LINE_STRIP, 0, v.size() / 4);
    // Draw min
    glVertexPointer(2, GL_FLOAT, 4*sizeof(float), (GLvoid*)(2*sizeof(float)));
    glDrawArrays(GL_LINE_STRIP, 0, v.size() / 4);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glLineWidth(1.0);

    // Unbind array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void TraceView::DrawBackdrop(QPoint pos, QPoint size)
{
//    glMatrixMode(GL_MODELVIEW);
//    glPushMatrix();
//    glLoadIdentity();
//    glMatrixMode(GL_PROJECTION);
//    glPushMatrix();
//    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glColor4f(1.0, 1.0, 1.0, 0.8);
    glBegin(GL_QUADS);
    glVertex2f(pos.x(), pos.y());
    glVertex2f(pos.x() + size.x(), pos.y());
    glVertex2f(pos.x() + size.x(), pos.y() + size.y());
    glVertex2f(pos.x(), pos.y() + size.y());
    glEnd();
    glPopAttrib();

    glDisable(GL_BLEND);

//    glPopMatrix();
//    glMatrixMode(GL_MODELVIEW);
//    glPopMatrix();
}

void TraceView::AddToPersistence(const GLVector &v)
{
    if(v.size() < 1) return;

    // Prep GL state, bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, persist_fbo);

    if(clear_persistence) {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        clear_persistence = false;
    }

    glViewport(0, 0, PERSIST_WIDTH, PERSIST_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LESS); // Only pass less
    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0);

    // Prep matrices
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glScalef(PERSIST_WIDTH, PERSIST_HEIGHT, 1.0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, PERSIST_WIDTH, 0, PERSIST_HEIGHT, -1, 1);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);

    // Reduce current color by blending a big full screen quad
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0, 0.0, 0.0, 0.02);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(0,0);
    glTexCoord2f(0,1); glVertex2f(0,1);
    glTexCoord2f(1,1); glVertex2f(1,1);
    glTexCoord2f(1,0); glVertex2f(1,0);
    glEnd();

    // Prep the trace, use decay rate to add to persistence
    glBlendFunc(GL_ONE, GL_ONE);
    glBindBuffer(GL_ARRAY_BUFFER, traceVBO);
    glColor3f(0.04, 0.04, 0.04);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float),
                  &v[0], GL_DYNAMIC_DRAW);
    glVertexPointer(2, GL_FLOAT, 0, (GLvoid*)0);

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Draw the trace
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_QUAD_STRIP, 0, v.size() / 2);
    glTranslatef(0, 0, -0.5);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_QUAD_STRIP, 0, v.size() / 2);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Revert GL state
    glDisable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    glLineWidth(1.0);
    glViewport(0, 0, width(), height());
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TraceView::AddToWaterfall(const GLVector &v)
{
    bool degenHack = false; // prevents degenerate polygons
    float x, z;

    // Gonna have to remove 'n' falls based on the actual
    //      view height / pixPerFall
    if(v.size() <= 0) return;

    // Remove all buffers past max size
    while (waterfall_verts.size() >= MAX_WATERFALL_LINES) {
        delete waterfall_verts.back();
        waterfall_verts.pop_back();
        delete waterfall_coords.back();
        waterfall_coords.pop_back();
    }

    if(v.size() * 0.25 > grat_sz.x() * 0.5) degenHack = true;

    waterfall_verts.insert(waterfall_verts.begin(), new GLVector);
    waterfall_coords.insert(waterfall_coords.begin(), new GLVector);

    waterfall_verts[0]->reserve((v.size() * 3) >> 1); // Re-look at these values
    waterfall_coords[0]->reserve(v.size() * 2);

    // Center samples, if/else on degen hack, to draw poly's greater than 1 pixel wide
    for(unsigned i = 0; i < v.size(); i += 4) {

        if(degenHack) {
            // Get max for three points, one on each side of sample in question
            x = v[i];
            z = bb_lib::max3(v[bb_lib::max2(1, (int)i-1)], // Sample to left
                    v[i+1], v[bb_lib::min2(i+3, (unsigned)v.size()-3)]);
        } else {
            // x & z for every sample
            x = v[i];
            z = v[i+1];
        }

        // Best place to clamp. This clamps the tex coord and pos
        // Must clamp height in waterfall because we don't have
        //   the luxury of clipping with a viewport.
        bb_lib::clamp(z, 0.0f, 1.0f);

        // Max Point
        waterfall_verts[0]->push_back(x);
        waterfall_verts[0]->push_back(0.0);
        waterfall_verts[0]->push_back(z);

        // Min Point
        waterfall_verts[0]->push_back(x);
        waterfall_verts[0]->push_back(0.0);
        waterfall_verts[0]->push_back(0.0);

        // Set Tex Coords
        waterfall_coords[0]->push_back(x);
        waterfall_coords[0]->push_back(z);

        waterfall_coords[0]->push_back(x);
        waterfall_coords[0]->push_back(0.0);
    }
}

void TraceView::ClearWaterfall()
{
    while(!waterfall_verts.empty()) {
        delete waterfall_verts.back();
        waterfall_verts.pop_back();
    }

    while(!waterfall_coords.empty()) {
        delete waterfall_coords.back();
        waterfall_coords.pop_back();
    }
}

/*
 * Draws our waterfall buffers
 * Three steps for both 2&3 Dimensional drawing
 * For each step, common setup first, then if/else for mode dependent stuff
 * Step 1) Setup
 * Step 2) VBO buffering and Drawing
 * Step 3) Break-Down/Revert GL state
 */
void TraceView::DrawWaterfall()
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, waterfall_tex);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(10, height() / 2 + 20, 0.0);
    glScalef(grat_ll.x() - 15, height() / 2 - 40, 1.0);

    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(0,0);
    glTexCoord2f(0,1); glVertex2f(0,1);
    glTexCoord2f(1,1); glVertex2f(1,1);
    glTexCoord2f(1,0); glVertex2f(1,0);
    glEnd();

    glPopMatrix();

    // Step 1 : Setup
    // Do Setup Based on waterfall mode
    //
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_LINE_SMOOTH);
//    glEnable(GL_BLEND);

    glPushAttrib(GL_VIEWPORT_BIT);
    glBindTexture(GL_TEXTURE_2D, waterfall_tex);

    if(waterfall_state == Waterfall2D) {
        // Create perfect fit viewport for 2D waterfall, auto clips for us
        glViewport(grat_ul.x(), height() / 2 + 20, grat_sz.x(), height() / 2 - 40);
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();
        glLoadIdentity();
        glScalef((float)grat_sz.x(), 1.0f, 1.0f);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, grat_sz.x(), 0, height() / 2 - 40, -1, 1);

    } else if(waterfall_state == Waterfall3D) {
        glViewport(0, grat_ul.y() + 50, width(), height() * 0.4);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glPerspective(90, (float)(0.4 * width() / height()), 0.1, 100);
        // Look At stuff
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        float ex, ey, ez;
        sphereToCart(theta, phi, rho, &ex, &ey, &ez);
        glLookAt(ex + 0.5, ey, ez + 0.5, /* Eye */
                  0.5, 0.0, 0.5, /* Center */
                  0, 0, 1); /* Up */
    }

    // Step 2 :
    // Data Buffering and drawing
    //
    for(unsigned i = 0; i < waterfall_verts.size(); i++) { // Loop through all traces
        std::vector<float> &r = *waterfall_verts[i]; // The trace
        std::vector<float> &t = *waterfall_coords[i]; // Trace tex coords

        // Buffer the data, each 2D and 3D handle setting pointers
        glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
        glBufferData(GL_ARRAY_BUFFER, t.size() * sizeof(float),
                     &t[0], GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, traceVBO);
        glBufferData(GL_ARRAY_BUFFER, r.size() * sizeof(float),
                     &r[0], GL_DYNAMIC_DRAW);

        if(waterfall_state == Waterfall2D) {
            // Reset glPointers, draw line across top of trace, then shift
            glLineWidth(2.0);
            glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
            glTexCoordPointer(2, GL_FLOAT, 16, (GLvoid*)0);
            glBindBuffer(GL_ARRAY_BUFFER, traceVBO);
            glVertexPointer(3, GL_FLOAT, 24, (GLvoid*)0);
            glDrawArrays(GL_LINE_STRIP, 0, r.size() / 6);
            glTranslatef(0.0, 2.0, 0.0);
            glLineWidth(1.0);

        } else if (waterfall_state == Waterfall3D) {
            // Main draw and pointers
            glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
            glTexCoordPointer(2, GL_FLOAT, 0, (GLvoid*)0);
            glBindBuffer(GL_ARRAY_BUFFER, traceVBO);
            glVertexPointer(3, GL_FLOAT, 0, (GLvoid*)0);
            glDrawArrays(GL_QUAD_STRIP, 0, r.size() / 3);

            // Draw the waterfall outline
            glDisable(GL_TEXTURE_2D);
            glColor3f(0.0, 0.0, 0.0);
            glBindBuffer(GL_ARRAY_BUFFER, traceVBO);
            glVertexPointer(3, GL_FLOAT, 24, (GLvoid*)0);
            glDrawArrays(GL_LINE_STRIP, 0, r.size() / 6);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_TEXTURE_2D);

            glTranslatef(0, 0.05f, 0);
        }
    }

    // Step 3 :
    // Clean up/Revert GL state
    //
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
    //glDisable(GL_LINE_SMOOTH);
    //glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
    glDisable(GL_DEPTH_TEST);
}
