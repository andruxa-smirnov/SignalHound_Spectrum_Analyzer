#include "status_bar.h"

BBStatusBar::BBStatusBar(QWidget *parent) :
    QStatusBar(parent)
{
    tempLabel = new Label();
    insertWidget(0, tempLabel, 2);
    tempLabel->setText("");

    cursorLoc = new Label();
    cursorLoc->setMinimumWidth(200);
    cursorLoc->setAlignment(Qt::AlignRight);
    cursorLoc->setText("");
    addPermanentWidget(cursorLoc, 0);

    deviceType = new Label();
    deviceType->setMinimumWidth(100);
    deviceType->setAlignment(Qt::AlignRight);
    deviceType->setText("No Device Connected");
    addPermanentWidget(deviceType, 0);

    diagnostics = new Label();
    diagnostics->setMinimumWidth(100);
    diagnostics->setAlignment(Qt::AlignRight);
    addPermanentWidget(diagnostics, 0);

    deviceInfo = new Label();
    deviceInfo->setMinimumWidth(100);
    deviceInfo->setAlignment(Qt::AlignRight);
    addPermanentWidget(deviceInfo, 0);
}
