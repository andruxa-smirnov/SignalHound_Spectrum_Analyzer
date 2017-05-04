#ifndef HARMONICS_CENTRAL_H
#define HARMONICS_CENTRAL_H

#include <thread>
#include <atomic>

#include "central_stack.h"
#include "harmonics_spectrum.h"

class HarmonicsCentral : public CentralWidget {
    Q_OBJECT

public:
    HarmonicsCentral(Session *sPtr,
                     QToolBar *mainToolBar,
                     QWidget *parent = 0,
                     Qt::WindowFlags f = 0);
    ~HarmonicsCentral();

    virtual void GetViewImage(QImage &image);
    virtual void StartStreaming();
    virtual void StopStreaming();
    virtual void ResetView();

    virtual Frequency GetCurrentCenterFreq() const;

protected:
    void resizeEvent(QResizeEvent *);

public slots:
    virtual void changeMode(int newState);
    void singlePressed() { if(sweepCount <= 0) sweepCount = 1; }
    void continuousPressed() { sweepCount = -1; }

private:
    void Reconfigure(SweepSettings &ss);
    void SweepThread();

    std::thread sweepThreadHandle;
    bool sweeping;
    bool reconfigure;
    std::atomic<int> sweepCount;

    Session *session_ptr;
    HarmonicsSpectrumPlot *plot;

private slots:
    void settingsChanged(const SweepSettings *ss);

signals:
    void updateView();

private:
    Q_DISABLE_COPY(HarmonicsCentral)
};

#endif // HARMONICS_CENTRAL_H
