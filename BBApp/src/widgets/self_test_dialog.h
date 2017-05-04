#ifndef SELF_TEST_DIALOG_H
#define SELF_TEST_DIALOG_H

#include <QDialog>

#include "model/device.h"
#include "entry_widgets.h"

class SelfTestDialog : public QDialog {
    Q_OBJECT

public:
    SelfTestDialog(Device *devicePtr, QWidget *parent = 0);
    ~SelfTestDialog();

private:
    Label *hbmResult, *hbmValue;
    Label *lbmResult, *lbmValue;
    Label *attenResult, *attenValue;
    Label *ifResult, *ifValue;
    Label *preampResult, *preampValue;

    SHPushButton *startTestButton;

    Device *device; // Does not own

private slots:
    void performTest();
};

#endif // SELF_TEST_DIALOG_H
