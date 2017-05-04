#include "gl_sub_view.h"

static const float linearInnerPoints[] =
{
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.1, 1.0, 0.1,
    0.0, 0.2, 1.0, 0.2,
    0.0, 0.3, 1.0, 0.3,
    0.0, 0.4, 1.0, 0.4,
    0.0, 0.5, 1.0, 0.5,
    0.0, 0.6, 1.0, 0.6,
    0.0, 0.7, 1.0, 0.7,
    0.0, 0.8, 1.0, 0.8,
    0.0, 0.9, 1.0, 0.9,
    0.0, 1.0, 1.0, 1.0,

    0.0, 0.0, 0.0, 1.0,
    0.1, 0.0, 0.1, 1.0,
    0.2, 0.0, 0.2, 1.0,
    0.3, 0.0, 0.3, 1.0,
    0.4, 0.0, 0.4, 1.0,
    0.5, 0.0, 0.5, 1.0,
    0.6, 0.0, 0.6, 1.0,
    0.7, 0.0, 0.7, 1.0,
    0.8, 0.0, 0.8, 1.0,
    0.9, 0.0, 0.9, 1.0,
    1.0, 0.0, 1.0, 1.0
};

static const size_t linearInnerCount = sizeof(linearInnerPoints) / sizeof(float);

static const float linearBorderPoints[] =
{
    0.0, 0.0, 1.0, 0.0,
    1.0, 0.0, 1.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    0.0, 1.0, 0.0, 0.0
};

static const size_t linearBorderCount = sizeof(linearBorderPoints) / sizeof(float);

GLSubView::GLSubView(Session *sPtr, QWidget *parent) :
    QGLWidget(parent),
    sessionPtr(sPtr)
{
    makeCurrent();

    initializeOpenGLFunctions();

    glGenBuffers(1, &gratVBO);
    glGenBuffers(1, &gratBorderVBO);

    glBindBuffer(GL_ARRAY_BUFFER, gratVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(linearInnerPoints),
                 linearInnerPoints, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, gratBorderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(linearBorderPoints),
                 linearBorderPoints, GL_DYNAMIC_DRAW);

    innerGratPoints = linearInnerCount;
    borderGratPoints = linearBorderCount;

    doneCurrent();

    setAutoFillBackground(false);
}

GLSubView::~GLSubView()
{
    makeCurrent();

    glDeleteBuffers(1, &gratVBO);
    glDeleteBuffers(1, &gratBorderVBO);

    doneCurrent();
}

//void GLSubView::DrawString(const QString &s,
//                           const GLFont &f,
//                           QPoint p,
//                           TextAlignment alignment)
//{
//    if(alignment == RIGHT_ALIGNED) {
//        p -= QPoint(f.GetTextWidth(s), 0);
//    } else if(alignment == CENTER_ALIGNED) {
//        p -= QPoint(f.GetTextWidth(s) / 2, 0);
//    }

//    renderText(p.x(), p.y(), 0, s, f.Font());
//}

//void GLSubView::DrawString(const QString &s,
//                           const GLFont &f,
//                           int x,
//                           int y,
//                           TextAlignment alignment)
//{
//    DrawString(s, f, QPoint(x, y), alignment);
//}

void GLSubView::DrawString(QPainter &painter,
                           const QString &s,
                           QPoint p,
                           TextAlignment alignment)
{

    if(alignment == RIGHT_ALIGNED) {
        p -= QPoint(painter.fontMetrics().width(s), 0);
    } else if(alignment == CENTER_ALIGNED) {
        p -= QPoint(painter.fontMetrics().width(s) / 2, 0);
    }

    p.setY(-p.y());
    painter.drawText(p, s);
}

void GLSubView::DrawString(QPainter &painter,
                           const QString &s,
                           int x,
                           int y,
                           TextAlignment alignment)
{
    DrawString(painter, s, QPoint(x, y), alignment);
}

void GLSubView::DrawGraticule()
{
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, width(), height());

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
    glDrawArrays(GL_LINES, 0, innerGratPoints / 2);

    if(GetSession()->prefs.graticule_stipple) {
        glDisable(GL_LINE_STIPPLE);
    }

    // Border
    glBindBuffer(GL_ARRAY_BUFFER, gratBorderVBO);
    glVertexPointer(2, GL_FLOAT, 0, INDEX_OFFSET(0));
    glDrawArrays(GL_LINES, 0, borderGratPoints / 2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glLineWidth(1.0);
    glPopAttrib();
}
