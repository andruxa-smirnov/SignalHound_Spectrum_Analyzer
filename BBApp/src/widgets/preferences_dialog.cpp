#include "preferences_dialog.h"

#include "../widgets/dock_panel.h"

///
// Preferences Panel
//
PreferencePanel::PreferencePanel(const QString &panelTitle,
                                 QWidget *parent) :
    QScrollArea(parent),
    title(panelTitle)
{
    this->move(0, 0);
    setObjectName("SH_PrefScroll");

    scrollWidget = new QWidget(this);
    scrollWidget->setObjectName("SH_Panel");
    //scrollWidget->setObjectName("SH_ScrollViewport");

    this->setWidget(scrollWidget);
    //this->setWidgetResizable(true);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

PreferencePanel::~PreferencePanel()
{

}

void PreferencePanel::AddPage(DockPage *page)
{
    connect(page, SIGNAL(tabUpdated()), this , SLOT(tabsChanged()));

    tabs.push_back(page);
    page->setParent(this->widget());

    //resize(DOCK_WIDTH, 25);
    tabsChanged();
}

void PreferencePanel::resizeEvent(QResizeEvent *)
{
    tabsChanged();
}

void PreferencePanel::tabsChanged()
{
    int height = 0;
    int pageHeight = 0;
    int viewportWidth = viewport()->width();

    for(int i = 0; i < tabs.size(); i++) {
        if(tabs[i]->TabIsOpen()) {
            pageHeight = tabs[i]->GetTotalHeight();
        } else {
            pageHeight = tabs[i]->TabHeight();
        }
        //tabs[i]->resize(DOCK_WIDTH, pageHeight);
        tabs[i]->resize(viewportWidth, pageHeight);
        tabs[i]->move(0, height);
        height += pageHeight + 1; // +1 is a small pad
    }

    scrollWidget->resize(viewportWidth, height);
}

PreferenceColorPanel::PreferenceColorPanel(const QString &panelTitle,
                                           QWidget *parent) :
    PreferencePanel(panelTitle, parent)
{
    DockPage *dockPage = new DockPage(tr("View"));

    traceWidth = new NumericEntry(tr("Trace Width"), 0.0, "");
    traceWidth->setToolTip(tr("Set the width of drawn sweeps"));
    graticuleWidth = new NumericEntry(tr("Graticule Width"), 0.0, "");
    graticuleWidth->setToolTip(tr("Set the drawing width of the graticule lines."));
    graticuleStipple = new CheckBoxEntry(tr("Graticule Dotted"));
    graticuleStipple->setToolTip(tr("Set whether the graticule is drawn with dotted lines."));

    dockPage->AddWidget(traceWidth);
    dockPage->AddWidget(graticuleWidth);
    dockPage->AddWidget(graticuleStipple);

    AddPage(dockPage);

    dockPage = new DockPage(tr("Colors"));

    background = new ColorEntry(tr("Background Color"));
    text = new ColorEntry(tr("Text Color"));
    graticule = new ColorEntry(tr("Graticule Color"));
    markerBorder = new ColorEntry(tr("Marker Border Color"));
    markerBackground = new ColorEntry(tr("Marker Background Color"));
    markerText = new ColorEntry(tr("Marker Text Color"));
    limitLines = new ColorEntry(tr("Limit Lines Color"));

    dockPage->AddWidget(background);
    dockPage->AddWidget(text);
    dockPage->AddWidget(graticule);
    dockPage->AddWidget(markerBorder);
    dockPage->AddWidget(markerBackground);
    dockPage->AddWidget(markerText);
    dockPage->AddWidget(limitLines);

    AddPage(dockPage);

    dockPage = new DockPage(tr("Sweep Settings"));

    sweepDelay = new NumericEntry(tr("Sweep Delay"), 0.0, tr("ms"));
    sweepDelay->setToolTip(tr("Slow down the sweep update rate with an added delay. "
                              "Can help to reduce CPU usage and decrease playback file sizes"));
    realTimeSweepTime = new NumericEntry(tr("Real-Time Frame Rate"), 0.0, tr("fps"));
    realTimeSweepTime->setToolTip(tr("Change the real-time update rate. "
                                     "15-30 fps is suggested"));

    dockPage->AddWidget(sweepDelay);
    dockPage->AddWidget(realTimeSweepTime);

    AddPage(dockPage);

    dockPage = new DockPage(tr("Playback Settings"));

    playbackDelay = new NumericEntry(tr("Playback Sweep Delay"), 0.0, tr("ms"));
    playbackDelay->setToolTip(tr("Set the speed at which a file is played back."));

    maxSaveFileSize = new NumericEntry(tr("Max Save File Size"), 0.0, tr("GB"));
    maxSaveFileSize->setToolTip(tr("Set the maximum allowable trace file to be recorded."));

    // Disable file size selection on 32-bit systems
#if !_WIN64
    maxSaveFileSize->setDisabled(true);
    maxSaveFileSize->setToolTip("Max File Size is 1GB on 32-bit systems");
#endif

    dockPage->AddWidget(playbackDelay);
    dockPage->AddWidget(maxSaveFileSize);

    AddPage(dockPage);
}

PreferenceColorPanel::~PreferenceColorPanel()
{

}

void PreferenceColorPanel::Populate(const Session *session)
{
    traceWidth->SetValue(session->prefs.trace_width);
    graticuleWidth->SetValue(session->prefs.graticule_width);
    graticuleStipple->SetChecked(session->prefs.graticule_stipple);

    const ColorPrefs &colors = session->colors;

    background->SetColor(colors.background);
    text->SetColor(colors.text);
    graticule->SetColor(colors.graticule);
    markerBorder->SetColor(colors.markerBorder);
    markerBackground->SetColor(colors.markerBackground);
    markerText->SetColor(colors.markerText);
    limitLines->SetColor(colors.limitLines);

    sweepDelay->SetValue(session->prefs.sweepDelay);
    realTimeSweepTime->SetValue(session->prefs.realTimeFrameRate);

    playbackDelay->SetValue(session->prefs.playbackDelay);
    maxSaveFileSize->SetValue(session->prefs.playbackMaxFileSize);
}

void PreferenceColorPanel::Apply(Session *session)
{
    double tWidth = traceWidth->GetValue();
    if(tWidth < 1.0) tWidth = 1.0;
    if(tWidth > 5.0) tWidth = 5.0;
    session->prefs.trace_width = tWidth;
    traceWidth->SetValue(tWidth);

    double gWidth = graticuleWidth->GetValue();
    if(gWidth < 1.0) gWidth = 1.0;
    if(gWidth > 5.0) gWidth = 5.0;
    session->prefs.graticule_width = gWidth;
    graticuleWidth->SetValue(gWidth);

    session->prefs.graticule_stipple = graticuleStipple->IsChecked();

    ColorPrefs &colors = session->colors;

    colors.background = background->GetColor();
    colors.text = text->GetColor();
    colors.graticule = graticule->GetColor();
    colors.markerBorder = markerBorder->GetColor();
    colors.markerBackground = markerBackground->GetColor();
    colors.markerText = markerText->GetColor();
    colors.limitLines = limitLines->GetColor();

    double sDelay = sweepDelay->GetValue();
    if(sDelay < 0.0) sDelay = 0.0;
    if(sDelay > 2048.0) sDelay = 2048.0;
    session->prefs.sweepDelay = int(sDelay);
    sweepDelay->SetValue(int(sDelay));

    int rtAccum = realTimeSweepTime->GetValue();
    if(rtAccum < 4) rtAccum = 4;
    if(rtAccum > 30) rtAccum = 30;
    session->prefs.realTimeFrameRate = rtAccum;
    realTimeSweepTime->SetValue(rtAccum);

    double pbDelay = playbackDelay->GetValue();
    if(pbDelay < 16.0) pbDelay = 16.0;
    if(pbDelay > 2048.0) pbDelay = 2048.0;
    session->prefs.playbackDelay = int(pbDelay);
    playbackDelay->SetValue(int(pbDelay));

    double maxFileSize = maxSaveFileSize->GetValue();
    if(maxFileSize < 1.0) maxFileSize = 1.0;
    if(maxFileSize > 128.0) maxFileSize = 128.0;
    session->prefs.playbackMaxFileSize = int(maxFileSize);
    maxSaveFileSize->SetValue(int(maxFileSize));
}

///
// Preference Trace Panel
//
PreferenceTracePanel::PreferenceTracePanel(const QString &panelTitle,
                                           QWidget *parent) :
    PreferencePanel(panelTitle, parent)
{

}

PreferenceTracePanel::~PreferenceTracePanel()
{

}

void PreferenceTracePanel::Populate(const Session *session)
{

}

void PreferenceTracePanel::Apply(Session *session)
{

}

///
// Preference Device Panel
//
PreferenceDevicePanel::PreferenceDevicePanel(const QString &panelTitle,
                                             QWidget *parent) :
    PreferencePanel(panelTitle, parent)
{

}

PreferenceDevicePanel::~PreferenceDevicePanel()
{

}

void PreferenceDevicePanel::Populate(const Session *session)
{

}

void PreferenceDevicePanel::Apply(Session *session)
{

}

// Official Preference Dialog
PreferenceDialog::PreferenceDialog(Session *session,
                                   QWidget *parent) :
    QDialog(parent),
    session_ptr(session)
{
    setObjectName("SH_Panel");

    setWindowTitle(tr("Preferences"));
    setFixedSize(PREF_RADIO_WIDTH + MAX_DOCK_WIDTH + 125, PREF_DLG_HEIGHT);

    groupBox = new QGroupBox(tr("Category"), this);
    groupBox->setObjectName("SH_PrefGroupBox");
    groupBox->move(0, 0);
    groupBox->resize(MAX_DOCK_WIDTH, 360);

    radioGroup = new QButtonGroup(this);

    connect(radioGroup, SIGNAL(buttonClicked(int)), this, SLOT(switchPanel(int)));

    panels.push_back(new PreferenceColorPanel(tr("General Settings"), this));
    //panels.push_back(new PreferenceTracePanel(tr("Sweep Settings"), this));
    //panels.push_back(new PreferenceDevicePanel(tr("Device Settings"), this));

    applyBtn = new SHPushButton(tr("Apply"), this);
    applyBtn->resize(90, 30);
    applyBtn->move(width() - 95, height() - 35);
    connect(applyBtn, SIGNAL(clicked()), this, SLOT(applyClicked()));

    cancelBtn = new SHPushButton(tr("Cancel"), this);
    cancelBtn->resize(90, 30);
    cancelBtn->move(width() - 190, height() - 35);
    connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));

    okBtn = new SHPushButton(tr("OK"), this);
    okBtn->resize(90, 30);
    okBtn->move(width() - 285, height() - 35);
    connect(okBtn, SIGNAL(clicked()), this, SLOT(accept()));

    // Resize and populate panels
    for(PreferencePanel *panel : panels) {
        panel->hide();
        panel->move(MAX_DOCK_WIDTH, 5);
        panel->resize(MAX_DOCK_WIDTH + 95, 350);
        panel->Populate(session_ptr);
    }

    // Use panels to create radio group
    const int radio_x = 10;
    int radio_y = 20;
    int radioIndex = 0;
    for(const PreferencePanel *panel : panels) {
        QRadioButton *radioBtn = new QRadioButton(panel->GetTitle(), this);
        radioBtn->setObjectName("SHPrefRadioButton");
        radioBtn->move(radio_x, radio_y);
        radioBtn->resize(200, 20);
        radioGroup->addButton(radioBtn, radioIndex++);
        radio_y += radioBtn->height();
    }

    switchPanel(0);
}

PreferenceDialog::~PreferenceDialog()
{

}

void PreferenceDialog::switchPanel(int panelIndex)
{
    // Force panel index of zero if it somehow falls outside the range
    //   of acceptable indices
    if(panelIndex < 0 || panelIndex >= panels.size()) {
        panelIndex = 0;
    }

    // Set radio button
    radioGroup->button(panelIndex)->setChecked(true);

    // Set proper panel
    for(int i = 0; i < panels.size(); i++) {
        if(i == panelIndex) {
            panels[i]->show();
        } else {
            panels[i]->hide();
        }
    }
}

void PreferenceDialog::applyClicked()
{
    for(PreferencePanel *panel : panels) {
        panel->Apply(session_ptr);
    }
}

