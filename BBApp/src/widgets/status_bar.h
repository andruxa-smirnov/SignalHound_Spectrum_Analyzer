#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include <QStatusBar>

#include "entry_widgets.h"

class BBStatusBar : public QStatusBar {
    Q_OBJECT

public:
    BBStatusBar(QWidget *parent = 0);
    ~BBStatusBar() {}

    void SetMessage(const QString &text) { tempLabel->setText(text); }
    void SetCursorPos(const QString &locStr) { cursorLoc->setText(locStr); }
    void SetDeviceType(const QString &text) { deviceType->setText(text); }
    void UpdateDeviceInfo(const QString &text) { deviceInfo->setText(text); }
    void SetDiagnostics(const QString &text) { diagnostics->setText(text); }

private:
    Label *cursorLoc;
    Label *tempLabel;  // Temporary info label
    Label *deviceType; // What device is connected
    Label *deviceInfo; // SN and FW of device
    Label *diagnostics; // Operating diagnostics
};

#endif // STATUS_BAR_H
