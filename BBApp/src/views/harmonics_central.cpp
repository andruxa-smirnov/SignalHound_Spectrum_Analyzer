#include "harmonics_central.h"
#include "model/session.h"

HarmonicsCentral::HarmonicsCentral(Session *sPtr,
                                   QToolBar *toolBar,
                                   QWidget *parent,
                                   Qt::WindowFlags f) :
    CentralWidget(parent, f),
    session_ptr(sPtr)
{
    sweeping = false;
    reconfigure = true;

    plot = new HarmonicsSpectrumPlot(session_ptr, this);
    plot->move(0, 0);
    connect(this, SIGNAL(updateView()), plot, SLOT(update()));

    connect(session_ptr->sweep_settings, SIGNAL(updated(const SweepSettings*)),
            this, SLOT(settingsChanged(const SweepSettings*)));
}

HarmonicsCentral::~HarmonicsCentral()
{
    StopStreaming();
}

void HarmonicsCentral::GetViewImage(QImage &image)
{
    image = plot->grabFrameBuffer();
}

void HarmonicsCentral::StartStreaming()
{
    sweeping = true;
    sweepThreadHandle = std::thread(&HarmonicsCentral::SweepThread, this);
}

void HarmonicsCentral::StopStreaming()
{
    if(sweepThreadHandle.joinable()) {
        sweeping = false;
        sweepThreadHandle.join();
    }
}

void HarmonicsCentral::ResetView()
{

}

Frequency HarmonicsCentral::GetCurrentCenterFreq() const
{
    return session_ptr->sweep_settings->Center();
}

void HarmonicsCentral::resizeEvent(QResizeEvent *)
{
    plot->resize(width(), height());
}

void HarmonicsCentral::changeMode(int newState)
{
    StopStreaming();

    session_ptr->sweep_settings->setMode((OperationalMode)newState);

    if(newState == MODE_HARMONICS) {
        StartStreaming();
    }
}

void HarmonicsCentral::Reconfigure(SweepSettings &ss)
{
    ss = *session_ptr->sweep_settings;
    reconfigure = false;
}

void HarmonicsCentral::SweepThread()
{
    // Special harmonic sweep settings and trace
    SweepSettings hss;
    Trace ht;
    Frequency center;

    while(sweeping) {
        if(reconfigure) {
            Reconfigure(hss);
            center = hss.Center();
        }

        for(int i = 0; i < 5; i++) {
            if(!sweeping || reconfigure) {
                break;
            }
            Frequency hCenter = center * (i+1);
            if(hCenter > device_traits::max_frequency()) {
                continue;
            }

            hss.setCenter(hCenter);
            hss.setSpan(200.0e3);
            hss.setAutoVbw(true);
            hss.setAutoRbw(true);

//            plot->traceLock[i].lock();
            session_ptr->device->Reconfigure(&hss, &ht);
            session_ptr->device->GetSweep(&hss, &ht);
            plot->harmonics[i].Update(ht);
//            plot->traceLock[i].unlock();

            emit updateView();
        }
    }

    session_ptr->device->Abort();
}

void HarmonicsCentral::settingsChanged(const SweepSettings *ss)
{
    reconfigure = true;
}
