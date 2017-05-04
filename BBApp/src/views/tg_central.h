#ifndef TG_CENTRAL_H
#define TG_CENTRAL_H

#include <thread>
#include <atomic>

#include <QResizeEvent>

#include "central_stack.h"
#include "tg_trace_view.h"

class Session;
class SweepSettings;

class TGCentral : public CentralWidget {
    Q_OBJECT

public:
    TGCentral(Session *sPtr,
              QToolBar *mainToolBar,
              QWidget *parent = 0,
              Qt::WindowFlags f = 0);
    ~TGCentral();

    virtual void GetViewImage(QImage &image);
    virtual void StartStreaming();
    virtual void StopStreaming();
    virtual void ResetView();

    virtual Frequency GetCurrentCenterFreq() const;

protected:
    void resizeEvent(QResizeEvent *);
    void keyPressEvent(QKeyEvent *);

public slots:
    virtual void changeMode(int newState);
    void singlePressed();
    void continuousPressed();

private:
    void SweepThread();

    std::thread sweepThreadHandle;
    bool sweeping;
    bool reconfigure;
    std::atomic<int> sweepCount;

    Session *session_ptr;
    TGPlot *plot;

private slots:
    void settingsChanged(const SweepSettings *ss) {
        reconfigure = true;
    }

signals:
    void updateView();

private:
    Q_DISABLE_COPY(TGCentral)
};

#endif // TG_CENTRAL_H
