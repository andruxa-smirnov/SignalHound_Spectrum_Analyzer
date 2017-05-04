#include "phase_noise_central.h"
#include "model/session.h"

PhaseNoiseCentral::PhaseNoiseCentral(Session *sPtr,
                                     QToolBar *mainToolBar,
                                     QWidget *parent,
                                     Qt::WindowFlags f) :
    CentralWidget(parent, f),
    session_ptr(sPtr)
{
    sweeping = false;
    reconfigure = true;

    startDecade = tempStartDecade = 2;
    stopDecade = tempStopDecade = 4;

    plot = new PhaseNoisePlot(session_ptr, this);
    plot->move(0, 0);
    connect(this, SIGNAL(updateView()), plot, SLOT(update()));

    tools.push_back(mainToolBar->addWidget(new FixedSpacer(QSize(10, 30))));

    Label *sweepLabel = new Label("Sweep");
    sweepLabel->resize(100, 25);
    tools.push_back(mainToolBar->addWidget(sweepLabel));

    tools.push_back(mainToolBar->addWidget(new FixedSpacer(QSize(10, 30))));

    startDecadeEntry = new ComboBox();
    startDecadeEntry->setFixedSize(150, 25);
    QStringList startList;
    startList << "10Hz" << "100Hz" << "1kHz" << "10kHz";
    startDecadeEntry->insertItems(0, startList);
    tools.push_back(mainToolBar->addWidget(startDecadeEntry));

    tools.push_back(mainToolBar->addWidget(new FixedSpacer(QSize(10, 30))));

    Label *startDecadeLabel = new Label("to");
    startDecadeLabel->resize(40, 25);
    tools.push_back(mainToolBar->addWidget(startDecadeLabel));

    tools.push_back(mainToolBar->addWidget(new FixedSpacer(QSize(10, 30))));

    stopDecadeEntry = new ComboBox();
    stopDecadeEntry->setFixedSize(150, 25);
    QStringList stopList;
    stopList << "1kHz" << "10kHz" << "100kHz" << "1MHz";
    stopDecadeEntry->insertItems(0, stopList);
    tools.push_back(mainToolBar->addWidget(stopDecadeEntry));

    connect(session_ptr->sweep_settings, SIGNAL(updated(const SweepSettings*)),
            this, SLOT(settingsChanged(const SweepSettings*)));
    connect(session_ptr->trace_manager, SIGNAL(updated()),
            this, SLOT(forceUpdateView()));

    startDecadeEntry->setCurrentIndex(startDecade);
    stopDecadeEntry->setCurrentIndex(stopDecade - 2);

    connect(startDecadeEntry, SIGNAL(activated(int)),
            this, SLOT(startDecadeChanged(int)));
    connect(stopDecadeEntry, SIGNAL(activated(int)),
            this, SLOT(stopDecadeChanged(int)));
}

PhaseNoiseCentral::~PhaseNoiseCentral()
{
    StopStreaming();
}

void PhaseNoiseCentral::GetViewImage(QImage &image)
{
    image = plot->grabFrameBuffer();
}

void PhaseNoiseCentral::StartStreaming()
{
    sweeping = true;
    sweepThreadHandle = std::thread(&PhaseNoiseCentral::SweepThread, this);
}

void PhaseNoiseCentral::StopStreaming()
{
    if(sweepThreadHandle.joinable()) {
        sweeping = false;
        sweepThreadHandle.join();
    }
}

void PhaseNoiseCentral::ResetView()
{

}

Frequency PhaseNoiseCentral::GetCurrentCenterFreq() const
{
    return session_ptr->sweep_settings->Center();
}

void PhaseNoiseCentral::resizeEvent(QResizeEvent *)
{
    plot->resize(width(), height());
}

void PhaseNoiseCentral::changeMode(int newState)
{
    StopStreaming();

    session_ptr->sweep_settings->setMode((OperationalMode)newState);

    if(newState == MODE_PHASE_NOISE) {
        StartStreaming();
    }
}

void PhaseNoiseCentral::Reconfigure(double &peakFreq, double &peakAmp)
{
    startDecade = tempStartDecade;
    stopDecade = tempStopDecade;
    plot->SetDecades(startDecade, stopDecade);
    SweepSettings peakSettings = *session_ptr->sweep_settings;
    reconfigure = false;

    // Find peak within 200kHz to determine the "true" center frequency
    // Phase noise measurements will be relative to this center
    Trace peakTrace;
    peakSettings.setSpan(199.0e3);
    peakSettings.setRBW(100.0);
    session_ptr->device->Reconfigure(&peakSettings, &peakTrace);
    session_ptr->device->GetSweep(&peakSettings, &peakTrace);
    //double peak;
    peakTrace.GetSignalPeak(&peakFreq, &peakAmp);

    //*session_ptr->sweep_settings->refLevel = peakAmp;

//    ss.setCenter(peak);
//    ss.setRefLevel(peakAmplitude);
//    ss.setAttenuation(0);
//    ss.setGain(0);
}

void PhaseNoiseCentral::SweepThread()
{
    Trace fullSweep(true); // Full sweep, combined partial sweeps
    Trace p; // Partial 1 decade sweep
    //SweepSettings ss; // Full sweep settings
    SweepSettings ps; // Partial sweep settings
    double peakFreq, peakAmp;

    std::vector<double> pTempMin, pTempMax;
    double pDataMin[1000], pDataMax[1000];

    while(sweeping) {
        if(reconfigure) {
            //Reconfigure(ss);
            Reconfigure(peakFreq, peakAmp);
            session_ptr->trace_manager->ClearAllTraces();

            // Partial sweeps need to have the peak amplitude setting and
            //   auto ranging
            ps.setRefLevel(peakAmp);
            ps.setGain(0);
            ps.setAttenuation(0);
        }

        fullSweep.SetSize(100 * (stopDecade - startDecade));
        fullSweep.SetUpdateRange(0, 100 * (stopDecade - startDecade));
        ps.setDetector(1);
        ps.setProcUnits(2);

        // Loop to generate one phase noise sweep
        for(int decade = startDecade + 1; decade < stopDecade + 1; decade++) {

            switch(decade) {
            case 0:
                // No longer doing 1Hz to 10Hz
                Q_ASSERT(false);
                break;
            case 1: // 10 Hz - 100 Hz, 1.6 Hz RBW, .46 Hz per bin
                ps.setStart(peakFreq + 8.0);
                ps.setStop(peakFreq + 110.0);
                ps.setRBW(1.6);
                ps.setVBW(1.6*.3);
                break;
            case 2: // 100 Hz - 1 KHz, 12.5 Hz RBW 3.7 Hz per bin
                ps.setStart(peakFreq + 90.0);
                ps.setStop(peakFreq + 1.2e3);
                ps.setRBW(12.5);
                ps.setVBW(12.5*.3);
                break;
            case 3: // 1 KHz - 10 KHz, 100 Hz RBW
                ps.setStart(peakFreq + 900.0);
                ps.setStop(peakFreq + 12.0e3);
                ps.setRBW(100.0);
                ps.setVBW(100.0*.3);
                break;
            case 4: // 10 KHz - 100 KHz, 1.6 KHz RBW
                ps.setStart(peakFreq + 8.0e3);
                ps.setStop(peakFreq + 120.0e3);
                ps.setRBW(1.6e3);
                ps.setVBW(1.6e3*.3);
                break;
            default: // 100 KHz - 1 MHz, 12.5 KHz RBW
                ps.setStart(peakFreq + 50.0e3);
                ps.setStop(peakFreq + 350.0e3);
                ps.setRBW(12.5e3);
                ps.setVBW(12.5e3);
                break;
            }

            double sweepStartFreq;
            pTempMin.clear();
            pTempMax.clear();

            if(decade < 5) {
                session_ptr->device->Reconfigure(&ps, &p);
                session_ptr->device->GetSweep(&ps, &p);
                sweepStartFreq = p.StartFreq();

                // Copy sweep to temp arrays
                for(int i = 0; i < p.Length(); i++) {
                    pTempMin.push_back(p.Min()[i]);
                    pTempMax.push_back(p.Max()[i]);
                }
            } else {
                // Sweep 100k - 1M with slow sweep
                session_ptr->device->Reconfigure(&ps, &p);
                session_ptr->device->GetSweep(&ps, &p);
                sweepStartFreq = p.StartFreq();
                // Copy sweep to temp arrays
                for(int i = 0; i < p.Length(); i++) {
                    pTempMin.push_back(p.Min()[i]);
                    pTempMax.push_back(p.Max()[i]);
                }

                for(int step = 0; step < 4; step++) {
                    ps.setCenter(peakFreq + 300.0e3 + 200.0e3 * step);
                    ps.setSpan(200.0e3);
                    session_ptr->device->Reconfigure(&ps, &p);
                    session_ptr->device->GetSweep(&ps, &p);

                    double stopOfPrev = sweepStartFreq + pTempMin.size() * p.BinSize();
                    int startIx = double(stopOfPrev - p.StartFreq()) / p.BinSize();
                    if(startIx < 0) {
                        //Q_ASSERT(0);
                        startIx = 0;
                    }

                    for(int i = startIx; i < p.Length(); i++) {
                        pTempMin.push_back(p.Min()[i]);
                        pTempMax.push_back(p.Max()[i]);
                    }
                }
            }

            double HzPerBin = p.BinSize();
            double HzRBW = 10.0 * log10(ps.RBW());
            int ptsPerDecade = 100;
            //double recipPtsPerDecade = 1.0 / ptsPerDecade;

            double startFreq = peakFreq + pow(10.0, decade);
            double stopFreq = peakFreq + pow(10.0, decade + 1);
            double span = stopFreq - startFreq;

            for(int i = 0; i < ptsPerDecade; i++) {
                double startFrac = pow(10.0, decade + double(i) / 101.0);
                double stopFrac = pow(10.0, decade + double(i+1) / 101.0);
                double startFracFreq = startFreq + startFrac * span;
                startFracFreq = peakFreq + startFrac;
                double stopFracFreq = startFreq + stopFrac * span;
                stopFracFreq = peakFreq + stopFrac;

                int startBin = floor((startFracFreq - sweepStartFreq) / HzPerBin + 0.5);
                int stopBin = floor((stopFracFreq - sweepStartFreq) / HzPerBin + 0.5);

                double thisMin = 0.0;
                double thisMax = 0.0;

                // Now average all valid bins together to get phase noise
                for(int j = startBin; j <= stopBin; j++) {
                    thisMin += pTempMin[j]; //p.Min()[j]; //pTempMin[j];
                    thisMax += pTempMax[j]; //p.Max()[j]; //pTempMax[j];
                }

                // Convert to dBm/Hz
                thisMin = (thisMin / (stopBin-startBin+1)) - peakAmp - HzRBW;
                thisMax = (thisMax / (stopBin-startBin+1)) - peakAmp - HzRBW;

                int thisIndex = i;// + (decade - startDecade) * ptsPerDecade;
                pDataMin[thisIndex] = thisMin;
                pDataMax[thisIndex] = thisMax;

                // Make sure we fill in any gaps. Overkill probably. Gets written over if not needed
                if(thisIndex < (ptsPerDecade/2)) {
                    pDataMin[thisIndex+1] = thisMin;
                    pDataMin[thisIndex+2] = thisMin;
                    pDataMax[thisIndex+1] = thisMax;
                    pDataMax[thisIndex+2] = thisMax;
                }

                // Make sure we fill in any gaps.  Overkill probably
                if(thisIndex < (ptsPerDecade/4)) {
                    pDataMin[thisIndex+3] = thisMin;
                    pDataMin[thisIndex+4] = thisMin;
                    pDataMin[thisIndex+5] = thisMin;
                    pDataMax[thisIndex+3] = thisMax;
                    pDataMax[thisIndex+4] = thisMax;
                    pDataMax[thisIndex+5] = thisMax;
                }
            }

            // Copy decade sweep into trace class
            int decadeStart = (decade - 1 - startDecade) * 100;
            for(int i = 0; i < ptsPerDecade; i++) {
                fullSweep.Min()[decadeStart + i] = pDataMin[i];
                fullSweep.Max()[decadeStart + i] = pDataMax[i];
            }
        }

        session_ptr->trace_manager->UpdateTraces(&fullSweep);
        emit updateView();
    }

    session_ptr->device->Abort();
}

void PhaseNoiseCentral::settingsChanged(const SweepSettings *ss)
{
    reconfigure = true;
}

void PhaseNoiseCentral::startDecadeChanged(int newStart)
{
    if(newStart >= stopDecade) {
        newStart = stopDecade - 1;
        startDecadeEntry->setCurrentIndex(newStart);
    }

    //startDecade = newStart;
    tempStartDecade = newStart;
    reconfigure = true;
}

void PhaseNoiseCentral::stopDecadeChanged(int newStop)
{
    newStop += 2;
    if(newStop <= startDecade) {
        newStop = startDecade + 1;
        stopDecadeEntry->setCurrentIndex(newStop - 2);
    }

    //stopDecade = newStop;
    tempStopDecade = newStop;
    reconfigure = true;
}

void PhaseNoiseCentral::forceUpdateView()
{
    emit updateView();
}
