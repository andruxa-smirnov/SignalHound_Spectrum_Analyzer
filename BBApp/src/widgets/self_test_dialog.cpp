#include "self_test_dialog.h"
#include "lib/sa_api.h"

SelfTestDialog::SelfTestDialog(Device *devicePtr, QWidget *parent) :
    QDialog(parent),
    device(devicePtr)
{
    setWindowTitle("Self-Test");
    setObjectName("SH_Page");
    setFixedSize(400, 350);

    Label *lbl;

    lbl = new Label("Ensure the RF input is connected direcly\n"
                    "to the Self Test BNC port before starting\n"
                    "the test. Once connected, press the start\n"
                    "test button.", this);
    lbl->move(15, 5);
    lbl->resize(370, 75);

    // Column labels
    lbl = new Label("Pass/Fail", this);
    lbl->move(200, 100);
    lbl->resize(100, 25);
    lbl = new Label("Reading", this);
    lbl->move(300, 100);
    lbl->resize(100, 25);

    lbl = new Label("High Band Mixer", this);
    lbl->move(15, 125);
    lbl->resize(130, 25);
    hbmResult = new Label("N/A", this);
    hbmResult->move(200, 125);
    hbmResult->resize(100, 25);
    hbmValue = new Label(" ", this);
    hbmValue->move(300, 125);
    hbmValue->resize(100, 25);

    lbl = new Label("Low Band Mixer", this);
    lbl->move(15, 150);
    lbl->resize(130, 25);
    lbmResult = new Label("N/A", this);
    lbmResult->move(200, 150);
    lbmResult->resize(100, 25);
    lbmValue = new Label(" ", this);
    lbmValue->move(300, 150);
    lbmValue->resize(100, 25);

    lbl = new Label("Attenuator", this);
    lbl->move(15, 175);
    lbl->resize(130, 25);
    attenResult = new Label("N/A", this);
    attenResult->move(200, 175);
    attenResult->resize(100, 25);
    attenValue = new Label(" ", this);
    attenValue->move(300, 175);
    attenValue->resize(100, 25);

    lbl = new Label("Second IF", this);
    lbl->move(15, 200);
    lbl->resize(130, 25);
    ifResult = new Label("N/A", this);
    ifResult->move(200, 200);
    ifResult->resize(100, 25);
    ifValue = new Label(" ", this);
    ifValue->move(300, 200);
    ifValue->resize(100, 25);

    lbl = new Label("Pre-amplifier", this);
    lbl->move(15, 225);
    lbl->resize(130, 25);
    preampResult = new Label("N/A", this);
    preampResult->move(200, 225);
    preampResult->resize(100, 25);
    preampValue = new Label(" ", this);
    preampValue->move(300, 225);
    preampValue->resize(100, 25);

    startTestButton = new SHPushButton("Start Test", this);
    startTestButton->move(15, 300);
    startTestButton->resize(200, 25);
    connect(startTestButton, SIGNAL(clicked()), this, SLOT(performTest()));
}

SelfTestDialog::~SelfTestDialog()
{

}

void SelfTestDialog::performTest()
{
    startTestButton->setText("Please Wait");
    startTestButton->setEnabled(false);

    saSelfTestResults results;
    saStatus status = saSelfTest(device->Handle(), &results);

    QString failStr = "<font color=red>Fail</font>";
    QString passStr = "<font color=green>Pass</font>";

    hbmResult->setText(results.highBandMixer ? passStr : failStr);
    hbmValue->setText(QString().sprintf("%.2f", results.highBandMixerValue));
    lbmResult->setText(results.lowBandMixer ? passStr : failStr);
    lbmValue->setText(QString().sprintf("%.2f", results.lowBandMixerValue));
    attenResult->setText(results.attenuator ? passStr : failStr);
    attenValue->setText(QString().sprintf("%.2f", results.attenuatorValue));
    ifResult->setText(results.secondIF ? passStr : failStr);
    ifValue->setText(QString().sprintf("%.2f", results.secondIFValue));
    preampResult->setText(results.preamplifier ? passStr : failStr);
    preampValue->setText(QString().sprintf("%.2f", results.preamplifierValue));

    startTestButton->setText("Start Test");
    startTestButton->setEnabled(true);
}
