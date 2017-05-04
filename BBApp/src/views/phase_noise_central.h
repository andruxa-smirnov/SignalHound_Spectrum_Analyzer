#ifndef PHASE_NOISE_CENTRAL_H
#define PHASE_NOISE_CENTRAL_H

#include <thread>
#include <atomic>

#include "central_stack.h"
#include "phase_noise_plot.h"

class PhaseNoiseCentral : public CentralWidget {
    Q_OBJECT

public:
    PhaseNoiseCentral(Session *sPtr,
                      QToolBar *mainToolBar,
                      QWidget *parent = 0,
                      Qt::WindowFlags f = 0);
    ~PhaseNoiseCentral();

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
    void Reconfigure(double &peakFreq, double &peakAmp);
    void SweepThread();

    std::thread sweepThreadHandle;
    bool sweeping;
    bool reconfigure;
    std::atomic<int> sweepCount;
    int startDecade, stopDecade;
    int tempStartDecade, tempStopDecade;
    //double peakAmplitude;

    Session *session_ptr;
    PhaseNoisePlot *plot;

    ComboBox *startDecadeEntry;
    ComboBox *stopDecadeEntry;

private slots:
    void settingsChanged(const SweepSettings *ss);
    void startDecadeChanged(int);
    void stopDecadeChanged(int);
    void forceUpdateView();

signals:
    void updateView();

private:
    Q_DISABLE_COPY(PhaseNoiseCentral)

};

#endif // PHASE_NOISE_CENTRAL_H
