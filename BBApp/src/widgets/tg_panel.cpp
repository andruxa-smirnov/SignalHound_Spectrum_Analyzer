#include "tg_panel.h"
#include "model/session.h"

#include <QMessageBox>
#include <QAction>

TgCtrlPanel::TgCtrlPanel(const QString &title, QWidget *parent, Session *session)
    : DockPanel(title, parent),
      session_ptr(session)
{
    main_page = new DockPage("Tracking Generator Controls");

    center = new FrequencyEntry("Center Freq", 1.0e9);
    freqStepSize = new FrequencyEntry("Freq Step", 1.0e6);
    DualButtonEntry *freqStep = new DualButtonEntry("Step Down", "Step Up");
    amp = new NumericEntry("Amplitude", -30.0, "dBm");
    ampStepSize = new NumericEntry("dB Step", 1.0, "dB");
    DualButtonEntry *ampStep = new DualButtonEntry("Step Down", "Step Up");

    main_page->AddWidget(center);
    main_page->AddWidget(freqStepSize);
    main_page->AddWidget(freqStep);
    main_page->AddWidget(amp);
    main_page->AddWidget(ampStepSize);
    main_page->AddWidget(ampStep);

    connect(center, SIGNAL(freqViewChanged(Frequency)), this, SLOT(changeFrequency(Frequency)));
    connect(freqStep, SIGNAL(leftPressed()), this, SLOT(stepFrequencyDown()));
    connect(freqStep, SIGNAL(rightPressed()), this, SLOT(stepFrequencyUp()));
    connect(amp, SIGNAL(valueChanged(double)), this, SLOT(changeAmplitude(double)));
    connect(ampStep, SIGNAL(leftPressed()), this, SLOT(stepAmplitudeDown()));
    connect(ampStep, SIGNAL(rightPressed()), this, SLOT(stepAmplitudeUp()));

    //QAction *a = toggleViewAction();
    //connect(a, SIGNAL(toggled(bool)), this, SLOT(becameVisible(bool)));

    enableAction = new QAction("Tracking Generator Controls", 0);
    enableAction->setCheckable(true);
    connect(enableAction, SIGNAL(toggled(bool)), this, SLOT(becameVisible(bool)));

    AppendPage(main_page);
}

TgCtrlPanel::~TgCtrlPanel()
{

}

void TgCtrlPanel::becameVisible(bool checked)
{
    if(checked) {
        if(!session_ptr->device->AttachTg()) {
            QMessageBox::warning(0, "Warning",
                                 "Tracking Generator Not Found\n"
                                 "Please connect your tracking generator\n"
                                 "and try again");
            enableAction->setChecked(false);
            return;
        }
        // If everything opened successfully,
        show();
        session_ptr->device->SetTg(center->GetFrequency(), amp->GetValue());

    } else {
        hide();
        session_ptr->device->SetTg(0.0, -30.0);
    }
}

void TgCtrlPanel::changeFrequency(Frequency f)
{
    session_ptr->device->SetTg(center->GetFrequency(),
                               amp->GetValue());
}

void TgCtrlPanel::stepFrequencyDown()
{
    Frequency f = center->GetFrequency();
    f -= freqStepSize->GetFrequency();

    if(!session_ptr->device->SetTg(f, amp->GetValue())) {

    } else {
        center->SetFrequency(f);
    }
}

void TgCtrlPanel::stepFrequencyUp()
{
    Frequency f = center->GetFrequency();
    f += freqStepSize->GetFrequency();

    if(!session_ptr->device->SetTg(f, amp->GetValue())) {

    } else {
        center->SetFrequency(f);
    }
}

void TgCtrlPanel::changeAmplitude(double a)
{
    session_ptr->device->SetTg(center->GetFrequency(),
                               amp->GetValue());
}

void TgCtrlPanel::stepAmplitudeDown()
{
    double a = amp->GetValue();
    a -= ampStepSize->GetValue();

    if(!session_ptr->device->SetTg(center->GetFrequency(), a)) {

    } else {
        amp->SetValue(a);
    }
}

void TgCtrlPanel::stepAmplitudeUp()
{
    double a = amp->GetValue();
    a += ampStepSize->GetValue();

    if(!session_ptr->device->SetTg(center->GetFrequency(), a)) {

    } else {
        amp->SetValue(a);
    }
}
