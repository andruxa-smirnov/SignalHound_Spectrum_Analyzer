#ifndef PERSISTENCE_VIEW_H
#define PERSISTENCE_VIEW_H

#include <QOpenGLFunctions>
#include <QGLContext>
#include "../lib/bb_lib.h"
#include "../model/session.h"

#include "../model/trace.h"

class PersistenceView : public QOpenGLFunctions {

public:
    PersistenceView(Persistence *p)
        : QOpenGLFunctions(QOpenGLContext::currentContext()),
          persistence_model(p)
    {
        persist_program = new GLProgram(persist_vs, persist_fs);
        persist_program->Compile(this);
    }

    ~PersistenceView()
    {

    }
    Persistence *persistence_model;
    GLProgram *persist_program;

    void Draw() {
//        //glViewport(0, 0, size().width(), size().height());

//        glMatrixMode(GL_MODELVIEW);
//        glLoadIdentity();
//        glMatrixMode(GL_PROJECTION);
//        glLoadIdentity();
//        glOrtho(0, 1.0, 0, 1.0, -1, 1);

//        //glClearColor(1.0,1.0,1.0,1.0);
//        //glClear(GL_COLOR_BUFFER_BIT);

//        glColor3f(1.0, 0.0, 0.0);
//        glBegin(GL_LINE_STRIP);
//        glVertex2f(.1,.1);
//        glVertex2f(.5, .9);
//        glVertex2f(.9, .1);
//        glVertex2f(.1, .1);
//        glEnd();

//        //renderText(0.1, 0.9, 0.0, "Print Test");

        static bool first_time = true;
        static GLuint persistTex;

        if(first_time) {
            glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
            // Render buffer start, build the tex we are writing to
            glGenTextures( 1 , &persistTex );
            glBindTexture( GL_TEXTURE_2D , persistTex );
            glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_WRAP_S , GL_CLAMP );
            glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_WRAP_T , GL_CLAMP );
            glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_MAG_FILTER , GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_MIN_FILTER , GL_LINEAR );
            glBindTexture( 1, 0 );

            first_time = false;
        }

        // Prep viewport
        //glViewport(grat_ll.x(), grat_ll.y(),
        //           grat_sz.x(), grat_sz.y());

        // Prep modelview
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Ortho
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, persistTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     persistence_model->Width(),
                     persistence_model->Height(), 0,
                     GL_RGBA, GL_FLOAT,
                     (unsigned char*)persistence_model->GetImage());

        // Draw a single quad over our grat
        glUseProgram(persist_program->Handle());
        //glBindTexture(GL_TEXTURE_2D, persistTex);
        //glGenerateMipmap(GL_TEXTURE_2D);

    //    glMatrixMode(GL_MODELVIEW);
    //    glPushMatrix();
    //    glLoadIdentity();
    //    glTranslatef((float)xMargin, (float)botYMargin, 0.0f);
    //    glScalef((float)tenXgrat, (float)tenYgrat, 1.0f);

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

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        //glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        //glLoadIdentity();

        glUseProgram(0);
    }
};

#endif // PERSISTENCE_VIEW_H
