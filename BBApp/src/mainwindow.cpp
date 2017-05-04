#include "mainwindow.h"
#include "views/sweep_central.h"
#include "widgets/audio_dialog.h"
#include "widgets/progress_dialog.h"
#include "widgets/preferences_dialog.h"
#include "widgets/measuring_receiver_dialog.h"
#include "widgets/if_output_dialog.h"
#include "widgets/self_test_dialog.h"

#include "version.h"

#include <QFile>
#include <QSplitter>
#include <QTreeView>
#include <QMenuBar>
#include <QInputDialog>
#include <QtPrintSupport>
#include <QMessageBox>

static const QString utilitiesTgControlString = "Tracking Generator Controls";
static const QString utilitiesIFOutputString = "SA124 IF Output";
static const QString utilitiesSelfTestString = "Self Test";

// Static status bar
BBStatusBar *MainWindow::status_bar;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      saveLayoutOnClose(true)
{
    setWindowTitle(tr("Spike: Signal Hound Spectrum Analyzer Software"));
    move(0, 0);
    const QRect r = QApplication::desktop()->screenGeometry();
    resize(r.width() - 20, r.height() - 80);

    session = new Session();

    // Side widgets have priority over top/bottom widgets
    this->setDockNestingEnabled(true);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);

    // Tab positions on the outside
    setTabPosition(Qt::RightDockWidgetArea, QTabWidget::East);
    setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::West);

    sweep_panel = new SweepPanel(tr("Sweep Settings"), this, session);
    sweep_panel->setObjectName("SweepSettingsPanel");
    connect(sweep_panel, SIGNAL(zeroSpanPressed()), this, SLOT(zeroSpanPressed()));

    measure_panel = new MeasurePanel(tr("Measurements"), this,
                                     session->trace_manager, session->sweep_settings);
    measure_panel->setObjectName("TraceMarkerPanel");

    demodPanel = new DemodPanel(tr("Demod Settings"), this, session->demod_settings);
    demodPanel->setObjectName("DemodSettingsPanel");

    tgPanel = new TgCtrlPanel(utilitiesTgControlString, this, session);
    tgPanel->setObjectName("TgControlPanel");

    addDockWidget(Qt::RightDockWidgetArea, sweep_panel);
    addDockWidget(Qt::LeftDockWidgetArea, measure_panel);
    addDockWidget(Qt::RightDockWidgetArea, demodPanel);
    addDockWidget(Qt::LeftDockWidgetArea, tgPanel);

    status_bar = new BBStatusBar();
    setStatusBar(status_bar);

    InitMenuBar();

    QToolBar *toolBar = new QToolBar();
    toolBar->setObjectName("SweepToolBar");
    toolBar->setMovable(true);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    toolBar->setFloatable(false);
    toolBar->layout()->setContentsMargins(0, 0, 0, 0);
    toolBar->layout()->setSpacing(0);

    centralStack = new CentralStack(this);
    setCentralWidget(centralStack);

    sweepCentral = new SweepCentral(session, toolBar);
    sweepCentral->EnableToolBarActions(false);
    centralStack->AddWidget(sweepCentral);

    demodCentral = new DemodCentral(session, toolBar);
    demodCentral->EnableToolBarActions(false);
    centralStack->AddWidget(demodCentral);

    harmonicCentral = new HarmonicsCentral(session, toolBar);
    centralStack->AddWidget(harmonicCentral);

    tgCentral = new TGCentral(session, toolBar);
    centralStack->AddWidget(tgCentral);

    phaseNoiseCentral = new PhaseNoiseCentral(session, toolBar);
    phaseNoiseCentral->EnableToolBarActions(false);
    centralStack->AddWidget(phaseNoiseCentral);

    // Add Single/continuous/preset button to toolbar
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBar->addWidget(spacer);
    SHPushButton *single_sweep = new SHPushButton(tr("    Single"), toolBar);
    single_sweep->setObjectName("SH_SCButton");
    single_sweep->setIcon(QIcon(":/icons/icon_single"));
    single_sweep->setFixedSize(100, TOOLBAR_H - 4);
    toolBar->addWidget(single_sweep);
    SHPushButton *continuous_sweep = new SHPushButton(tr("     Auto"), toolBar);
    continuous_sweep->setIcon(QIcon(":/icons/icon_continuous"));
    continuous_sweep->setObjectName("SH_SCButton");
    continuous_sweep->setFixedSize(100, TOOLBAR_H - 4);
    toolBar->addWidget(continuous_sweep);

    connect(single_sweep, SIGNAL(clicked()), sweepCentral, SLOT(singleSweepPressed()));
    connect(single_sweep, SIGNAL(clicked()), demodCentral, SLOT(singlePressed()));
    connect(single_sweep, SIGNAL(clicked()), harmonicCentral, SLOT(singlePressed()));
    connect(single_sweep, SIGNAL(clicked()), tgCentral, SLOT(singlePressed()));
    connect(single_sweep, SIGNAL(clicked()), phaseNoiseCentral, SLOT(singlePressed()));
    connect(continuous_sweep, SIGNAL(clicked()), sweepCentral, SLOT(continuousSweepPressed()));
    connect(continuous_sweep, SIGNAL(clicked()), demodCentral, SLOT(autoPressed()));
    connect(continuous_sweep, SIGNAL(clicked()), harmonicCentral, SLOT(continuousPressed()));
    connect(continuous_sweep, SIGNAL(clicked()), tgCentral, SLOT(continuousPressed()));
    connect(continuous_sweep, SIGNAL(clicked()), phaseNoiseCentral, SLOT(continuousPressed()));

    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));
    toolBar->addSeparator();
    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));

    QPushButton *preset_button = new QPushButton(tr("Preset"), toolBar);
    preset_button->setObjectName("BBPresetButton");
    preset_button->setFixedSize(120, TOOLBAR_H - 4);
    toolBar->addWidget(preset_button);

    connect(preset_button, SIGNAL(clicked()), this, SLOT(presetDevice()));

    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));

    addToolBar(toolBar);
    RestoreState();
    toolBar->show();

    ChangeMode(MODE_SWEEPING);

    connectDeviceUponOpen();
}

MainWindow::~MainWindow()
{
    // Careful here, centralStack uses session
    // Deleting session first results in crash
    delete centralStack;
    delete session;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    // Hide all toolbars before saving window state
    // For now, prevents all toolbars from being shown on startup
    QList<QToolBar*> toolBars = findChildren<QToolBar*>();
    for(QToolBar *tb : toolBars) {
        tb->hide();
    }

    SaveState();
    QMainWindow::closeEvent(e);
}

void MainWindow::InitMenuBar()
{
    main_menu = new QMenuBar();

    // File Menu
    file_menu = main_menu->addMenu(tr("File"));
    //file_menu->addAction(tr("New"));
    //file_menu->addSeparator();
    file_menu->addAction(tr("Print"), this, SLOT(printView()));
    file_menu->addAction(tr("Save as Image"), this, SLOT(saveAsImage()));
    file_menu->addSeparator();
    QMenu *import_menu = file_menu->addMenu(tr("Import"));
    QMenu *import_path_loss = import_menu->addMenu(tr("Path-Loss"));
    import_path_loss->addAction(tr("Import Path-Loss Table"), session->trace_manager, SLOT(importPathLoss()));
    import_path_loss->addAction(tr("Clear Path-Loss Table"), session->trace_manager, SLOT(clearPathLoss()));
    QMenu *import_limits = import_menu->addMenu(tr("Limit-Lines"));
    import_limits->addAction(tr("Import Limit-Lines"), session->trace_manager, SLOT(importLimitLines()));
    import_limits->addAction(tr("Clear Limit-Lines"), session->trace_manager, SLOT(clearLimitLines()));
    file_menu->addSeparator();

    connect_menu = file_menu->addMenu("Connect");
    connect(connect_menu, SIGNAL(aboutToShow()), this, SLOT(populateConnectMenu()));
    connect(connect_menu, SIGNAL(triggered(QAction*)), this, SLOT(connectDevice(QAction*)));

    file_menu->addAction(tr("Disconnect Device"), this, SLOT(disconnectDevice()));
    connect(file_menu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowFileMenu()));
    file_menu->addSeparator();
    file_menu->addAction(tr("Exit"), this, SLOT(close()));

    // Edit Menu
    QMenu *edit_menu = main_menu->addMenu(tr("Edit"));
    edit_menu->addAction(tr("Restore Default Layout"), this, SLOT(restoreDefaultLayout()));
    edit_menu->addSeparator();
    edit_menu->addAction(tr("Title"), this, SLOT(setTitle()));
    edit_menu->addAction(tr("Clear Title"), this, SLOT(clearTitle()));
    QMenu *view_color_menu = edit_menu->addMenu(tr("Colors"));
    view_color_menu->addAction(tr("Load Default Colors"), this, SLOT(loadDefaultColors()));
    view_color_menu->addAction(tr("Load Printer Friendly Colors"),
                               this, SLOT(loadPrinterFriendlyColors()));
    view_color_menu->addAction(tr("Save as Default"), this, SLOT(saveAsDefaultColorScheme()));
    edit_menu->addSeparator();

    QMenu *style_menu = edit_menu->addMenu("Program Style");
    style_menu->addAction(tr("Light"), this, SLOT(loadStyleLight()));
    style_menu->addAction(tr("Dark"), this, SLOT(loadStyleDark()));
    style_menu->addAction(tr("Blue"), this, SLOT(loadStyleBlue()));

    edit_menu->addSeparator();
    edit_menu->addAction(tr("Preferences"), this, SLOT(showPreferencesDialog()));

    // Preset Menu
    preset_menu = main_menu->addMenu(tr("Presets"));

    preset_menu->addAction(tr("Load Default Settings"), this, SLOT(loadDefaultSettings()));
    preset_menu->addSeparator();
    preset_load = preset_menu->addMenu(tr("Load"));
    preset_save = preset_menu->addMenu(tr("Save"));
    preset_menu->addSeparator();
    preset_name = preset_menu->addMenu(tr("Rename"));

    for(int i = 0; i < PRESET_COUNT; i++) {
        QAction *load_action = preset_load->addAction(tr(""));
        QAction *save_action = preset_save->addAction(tr(""));
        QAction *name_action = preset_name->addAction(tr(""));

        load_action->setData(int(i));
        save_action->setData(int(i));
        name_action->setData(int(i));

        QString numeric;
        numeric.sprintf("%d", i+1);
        load_action->setShortcut(QKeySequence("Ctrl+" + numeric));
    }

    connect(preset_menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowPresetMenu()));
    connect(preset_load, SIGNAL(triggered(QAction*)), this, SLOT(loadPreset(QAction*)));
    connect(preset_save, SIGNAL(triggered(QAction*)), this, SLOT(savePreset(QAction*)));
    connect(preset_name, SIGNAL(triggered(QAction*)), this, SLOT(renamePreset(QAction*)));
    connect(preset_menu, SIGNAL(aboutToShow()), this, SLOT(loadPresetNames()));

    settings_menu = main_menu->addMenu(tr("Settings"));

    timebase_menu = settings_menu->addMenu(tr("Reference"));
    QAction *timebase_action;
    timebase_action = timebase_menu->addAction(tr("Internal"));
    timebase_action->setData(TIMEBASE_INTERNAL);
    timebase_action->setCheckable(true);
    timebase_action = timebase_menu->addAction(tr("External Sin Wave"));
    timebase_action->setData(TIMEBASE_EXT_AC);
    timebase_action->setCheckable(true);
    timebase_action = timebase_menu->addAction(tr("External CMOS-TTL"));
    timebase_action->setData(TIMEBASE_EXT_DC);
    timebase_action->setCheckable(true);
    connect(timebase_menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowTimebaseMenu()));
    connect(timebase_menu, SIGNAL(triggered(QAction*)), this, SLOT(timebaseChanged(QAction*)));

    QAction *sr_action = settings_menu->addAction(tr("Spur Reject"));
    sr_action->setCheckable(true);
    connect(sr_action, SIGNAL(triggered(bool)), session->sweep_settings, SLOT(setRejection(bool)));
    QAction *manual_gain = settings_menu->addAction(tr("Enable Manual Gain/Atten"));
    manual_gain->setCheckable(true);
    connect(manual_gain, SIGNAL(toggled(bool)), sweep_panel, SLOT(enableManualGainAtten(bool)));
    connect(manual_gain, SIGNAL(toggled(bool)), demodPanel, SLOT(enableManualGainAtten(bool)));
    connect(settings_menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowSettingsMenu()));

    // Mode Select Menu
    mode_menu = main_menu->addMenu(tr("Analysis Mode"));

    QActionGroup *mode_action_group = new QActionGroup(mode_menu);
    QAction *mode_action;
    mode_action = mode_menu->addAction(tr("Idle"));
    mode_action->setData(MODE_IDLE);
    mode_action->setCheckable(true);
    mode_action_group->addAction(mode_action);

    mode_action = mode_menu->addAction(tr("Sweep"));
    mode_action->setData(MODE_SWEEPING);
    mode_action->setCheckable(true);
    mode_action->setChecked(true);
    mode_action_group->addAction(mode_action);

    mode_action = mode_menu->addAction(tr("Real-Time"));
    mode_action->setData(MODE_REAL_TIME);
    mode_action->setCheckable(true);
    mode_action_group->addAction(mode_action);

    mode_action = mode_menu->addAction(tr("Zero-Span"));
    mode_action->setData(MODE_ZERO_SPAN);
    mode_action->setCheckable(true);
    mode_action_group->addAction(mode_action);

    mode_action = mode_menu->addAction("Harmonics Viewer");
    mode_action->setData(MODE_HARMONICS);
    mode_action->setCheckable(true);
    mode_action_group->addAction(mode_action);

    mode_action = mode_menu->addAction("Scalar Network Analysis");
    mode_action->setData(MODE_NETWORK_ANALYZER);
    mode_action->setCheckable(true);
    mode_action_group->addAction(mode_action);

    mode_action = mode_menu->addAction("Phase Noise Plot");
    mode_action->setData(MODE_PHASE_NOISE);
    mode_action->setCheckable(true);
    mode_action_group->addAction(mode_action);

    connect(mode_action_group, SIGNAL(triggered(QAction*)),
            this, SLOT(modeChanged(QAction*)));
    connect(mode_menu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowModeMenu()));

    utilities_menu = main_menu->addMenu(tr("Utilities"));
    utilities_menu->addAction(tr("Audio Player"), this, SLOT(startAudioPlayer()));
    utilities_menu->addAction(tr("Measuring Receiever"), this, SLOT(startMeasuringReceiever()));
    utilities_menu->addAction(tgPanel->toggleEnableAction());
    utilities_menu->addAction(utilitiesIFOutputString, this, SLOT(startSA124IFOutput()));
    utilities_menu->addAction(utilitiesSelfTestString, this, SLOT(startSelfTest()));
    connect(utilities_menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowUtilitiesMenu()));

    help_menu = main_menu->addMenu(tr("Help"));
    QAction *help_action = help_menu->addAction(tr("About"));
    connect(help_action, SIGNAL(triggered()),
            this, SLOT(showAboutBox()));

    setMenuBar(main_menu);
}

// Save the application state
// Called in MainWindow::closeEvent()
void MainWindow::SaveState()
{
    if(!saveLayoutOnClose) {
        return;
    }

    // Use .ini files in AppData folder to save settings
    //  instead of the registry, user scope
    QSettings settings(QSettings::IniFormat,
                       QSettings::UserScope,
                       "SignalHound",
                       "Layout");

    settings.setValue("MainWindow/State", saveState());
    settings.setValue("MainWindow/Geometry", geometry());
    settings.setValue("MainWindow/Maximized", isMaximized());

    sweep_panel->SaveState(settings);
    measure_panel->SaveState(settings);
    demodPanel->SaveState(settings);
    tgPanel->SaveState(settings);
}

/*
 * Restore previously saved mainwindow state
 * Do nothing if the save .ini file doesn't exist
 */
void MainWindow::RestoreState()
{   
    // Use .ini files in AppData folder to save settings
    //  instead of the registry, user scope
    QSettings settings(QSettings::IniFormat,
                       QSettings::UserScope,
                       "SignalHound",
                       "Layout");

    QVariant value;

    // Get widget positions
    value = settings.value("MainWindow/State");
    if(value.isValid()) {
        restoreState(value.toByteArray());
    }

    // Get window geometry
    value = settings.value("MainWindow/Geometry");
    if(value.isValid()) {
        setGeometry(value.toRect());
    }

    // Reset maximized
    value = settings.value("MainWindow/Maximized");
    if(value.isValid()) {
        if(value.toBool()) {
            showMaximized();
        }
    }

    sweep_panel->RestoreState(settings);
    measure_panel->RestoreState(settings);
    demodPanel->RestoreState(settings);
    tgPanel->RestoreState(settings);
    tgPanel->setVisible(false);
}

void MainWindow::restoreDefaultLayout()
{
    // Get the file name of the layout .inf file
    QString fileName;
    {
        // QSettings must be destroyed before attempting to open
        //   QFile?
        QSettings settings(QSettings::IniFormat,
                           QSettings::UserScope,
                           "SignalHound",
                           "Layout");
        fileName = settings.fileName();
    }

    QFile file(fileName);
    if(file.exists()) {
        file.remove();
    }

    // Doesn't technically matter if the file existed to delete or not
    // Tell user to restart program
    saveLayoutOnClose = false;
    QMessageBox::information(this, "Restart Application",
                             "The default application layout will be restored "
                             "the next time the application is started.");
}

// Hide Connect/Disconnect Device Options
void MainWindow::aboutToShowFileMenu()
{
    QList<QAction*> a_list = file_menu->actions();
    if(a_list.length() <= 0) return;

    for(QAction *a : a_list) {
        if(a->text() == tr("Disconnect Device")) {
            a->setDisabled(!session->device->IsOpen());
        }
    }

    connect_menu->setDisabled(session->device->IsOpen());
}

void MainWindow::aboutToShowSettingsMenu()
{
    for(QAction *a : settings_menu->actions()) {
        if(a->text() == tr("Spur Reject")) {
            a->setChecked(session->sweep_settings->Rejection());
        }
    }
}

void MainWindow::aboutToShowModeMenu()
{
    int current_mode = session->sweep_settings->Mode();
    bool isOpen = session->device->IsOpen();
    DeviceType devType = session->device->GetDeviceType();

    QList<QAction*> a_list = mode_menu->actions();

    // Set all enabled/disabled
    for(QAction *a : a_list) {
        a->setEnabled(isOpen);

        if(!isOpen) {
            continue;
        }

        // Additional checks/manipulations only if device is present
        if(a->data() == MODE_NETWORK_ANALYZER) {
            a->setEnabled(session->device->IsCompatibleWithTg());
        }

        if(a->data() == MODE_PHASE_NOISE) {
            a->setEnabled(devType != DeviceTypeBB60A
                    && devType != DeviceTypeBB60C);
        }

        if(a->data() == current_mode) {
            a->setChecked(true);
        }
    }
}

// Disable utilities that require a device to be open
void MainWindow::aboutToShowUtilitiesMenu()
{
    bool isOpen = session->device->IsOpen();

    for(QAction *a : utilities_menu->actions()) {
        a->setEnabled(isOpen);

        if(!isOpen) continue;

        if(a->text() == utilitiesTgControlString) {
            a->setEnabled(session->device->IsCompatibleWithTg());
        }

        if(a->text() == utilitiesIFOutputString) {
            a->setEnabled(session->device->GetDeviceType() == DeviceTypeSA124);
        }

        if(a->text() == utilitiesSelfTestString) {
            a->setEnabled(session->device->CanPerformSelfTest());
        }
    }
}

void MainWindow::OpenDevice(QMap<QString, QVariant> devInfoMap)
{
    QString openLabel;
    if(devInfoMap["Series"].toInt() == saSeries) {
        openLabel = "Connecting Device\nEstimated 6 seconds\n";
    } else {
        openLabel = "Connecting Device\nEstimated 3 seconds";
    }

    SHProgressDialog pd(openLabel, this);
    pd.show();

    Device *device;
    if(devInfoMap["Series"].toInt() == saSeries) {
        device = new DeviceSA(&session->prefs);
    } else {
        device = new DeviceBB60A(&session->prefs);
    }

    // Replace the old device with the new one
    Device *tempDevice = session->device;
    session->device = device;
    delete tempDevice;

    QEventLoop el;
    std::thread t = std::thread(&MainWindow::OpenDeviceInThread,
                                this,
                                &el,
                                device,
                                devInfoMap["SerialNumber"].toInt());
    el.exec();
    if(t.joinable()) {
        t.join();
    }

    deviceConnected(session->device->IsOpen());

    return;
}

void MainWindow::OpenDeviceInThread(QEventLoop *el, Device *device, int serialToOpen)
{
    device->OpenDeviceWithSerial(serialToOpen);

    while(!el->isRunning()) {
        Sleep(1);
    }
    el->exit();
}

void MainWindow::Preset()
{
    // Stop any sweeping
    centralStack->CurrentWidget()->changeMode(MODE_IDLE);
    centralStack->CurrentWidget()->ResetView();

    //ChangeMode(OperationalMode(BB_IDLE));

    session->LoadDefaults();
    session->trace_manager->Reset();

    QMap<QString, QVariant> devInfoMap;
    DeviceType devType = session->device->GetDeviceType();
    if(devType == DeviceTypeBB60A || devType == DeviceTypeBB60C) {
        devInfoMap["Series"] = bbSeries;
        if(devType == DeviceTypeBB60A) {
            devInfoMap["SerialNumber"] = 0;
        } else {
            devInfoMap["SerialNumber"] = session->device->SerialNumber();
        }
    } else {
        devInfoMap["Series"] = saSeries;
        devInfoMap["SerialNumber"] = session->device->SerialNumber();
    }

    SHProgressDialog pd("Preset Device\n"
                        "Estimated 3 seconds", this);
    pd.show();

    QEventLoop el;
    std::thread t = std::thread(&MainWindow::PresetDeviceInThread, this, &el);
    el.exec();
    if(t.joinable()) {
        t.join();
    }

    session->LoadDefaults();

    pd.hide();
    OpenDevice(devInfoMap);

    return;

}

void MainWindow::PresetDeviceInThread(QEventLoop *el)
{
    session->device->Preset();
    while(!el->isRunning()) {
        Sleep(1);
    }
    el->exit();
}

// Only called once upon opening the program
// If a single device is connected open it, else
//   report a number messages to the user.
void MainWindow::connectDeviceUponOpen()
{
    // Look for no devices or more than one device
    auto list = session->device->GetDeviceList();
    if(list.size() == 0) {
        QMessageBox::warning(this, "Signal Hound", "No Device Found.");
        return;
    }
    if(list.size() > 1) {
        QMessageBox::warning(this, "Message", "More than one device found. "
                             "Use the File->Connect menu to select which device to open.");
        return;
    }

    DeviceConnectionInfo item = list.at(0);
    QMap<QString, QVariant> devInfo;

    if(item.series == saSeries) {
        devInfo["Series"] = saSeries;
    } else {
        devInfo["Series"] = bbSeries;
    }
    devInfo["SerialNumber"] = item.serialNumber;

    OpenDevice(devInfo);
}

void MainWindow::connectDevice(QAction *a)
{
    QMap<QString, QVariant> infoMap = a->data().toMap();
    OpenDevice(infoMap);
}

// Add all available devices to the connect menu
void MainWindow::populateConnectMenu()
{
    connect_menu->clear();
    auto list = session->device->GetDeviceList();

    if(list.empty()) {
        QAction *a = connect_menu->addAction("No Devices Found");
        a->setEnabled(false);
        return;
    }

    for(auto &item : list) {
        QString label;
        QMap<QString, QVariant> infoMap;

        if(item.series == saSeries) {
            label += "SA44/124:  ";
            infoMap["Series"] = saSeries;
        } else {
            label += "BB60C:  ";
            infoMap["Series"] = bbSeries;
        }

        if(item.serialNumber == 0) {
            label = "BB60A";
            infoMap["SerialNumber"] = 0;
        } else {
            label += QVariant(item.serialNumber).toString();
            infoMap["SerialNumber"] = item.serialNumber;
        }

        QAction *a = connect_menu->addAction(label);
        a->setData(infoMap);
    }
}

// File Menu Disconnect Device
// Can also be called from the device recieving a disconnect error
//   during normal operation
void MainWindow::disconnectDevice()
{
    // Stop any sweeping
    centralStack->CurrentWidget()->changeMode(BB_IDLE);
    centralStack->CurrentWidget()->ResetView();

    ChangeMode(OperationalMode(BB_IDLE));

    session->LoadDefaults();
    session->trace_manager->Reset();

    session->device->CloseDevice();

    status_bar->SetMessage("");
    status_bar->SetDeviceType("No Device Connected");
    status_bar->SetDiagnostics("");
    status_bar->UpdateDeviceInfo("");
}

// Call disconnect AND provide user warning
void MainWindow::forceDisconnectDevice()
{
    disconnectDevice();
    QMessageBox::warning(0, "Connectivity Issues", "Device Connection Issues Detected, Ensure the"
                         " device is connected before opening again via the File Menu");
}

// Preset Button Disconnect Device
void MainWindow::presetDevice()
{
    Preset();
}

void MainWindow::deviceConnected(bool success)
{
    QString device_string;

    if(success) {
        device_string = "Serial - " + session->device->SerialString() +
                "    Firmware " + session->device->FirmwareString();

        status_bar->SetDeviceType(session->device->GetDeviceString());
        status_bar->UpdateDeviceInfo(device_string);

        device_traits::set_device_type(session->device->GetDeviceType());
        session->LoadDefaults();
        connect(session->device, SIGNAL(connectionIssues()),
                this, SLOT(forceDisconnectDevice()));

        sweep_panel->DeviceConnected(session->device->GetDeviceType());

        ChangeMode(MODE_SWEEPING);
        centralStack->CurrentWidget()->changeMode(BB_SWEEPING);
    } else {
        QMessageBox::information(this, tr("Connection Status"),
                                 session->device->GetLastStatusString());
        status_bar->SetDeviceType("No Device Connected");
    }
}

void MainWindow::printView()
{
    QPrinter printer;
    printer.setOrientation(QPrinter::Landscape);

    QPrintDialog dialog(&printer, this);
    if(dialog.exec() != QDialog::Accepted) {
        return;
    }

    QPainter painter(&printer);
    QRect rect = painter.viewport();

    QImage image;
    centralStack->CurrentWidget()->GetViewImage(image);

    QSize size = image.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter.setViewport(rect.x(), rect.y(),
                        size.width()-5, size.height()-5);
    painter.setWindow(image.rect());
    painter.drawImage(2, 2, image);
}

void MainWindow::saveAsImage()
{
    QString file_name = QFileDialog::getSaveFileName(this,
                                                     tr("Image Save Name"),
                                                     sh::GetDefaultImageDirectory(),
                                                     tr("Images (*.png *.jpg *.bmp)"));
    if(file_name.isNull()) return;

    QImage image;
    centralStack->CurrentWidget()->GetViewImage(image);

    image.save(file_name);

    sh::SetDefaultImageDirectory(QFileInfo(file_name).absoluteDir().absolutePath());
}

void MainWindow::setTitle()
{
    bool ok;
    // Create Title dialog with existing title present
    QString new_title =
            QInputDialog::getText(this,
                                  tr("Set Title"),
                                  tr("Enter Title (Between 3-63 characters)"),
                                  QLineEdit::Normal,
                                  session->title,
                                  &ok);

    if(ok) {
        session->SetTitle(new_title);
    }
}

void MainWindow::loadDefaultColors()
{
    session->colors.LoadDefaults();
}

void MainWindow::loadPrinterFriendlyColors()
{
    session->colors.LoadPrinterFriendly();
}

void MainWindow::saveAsDefaultColorScheme()
{

}

void MainWindow::loadDefaultSettings()
{
    centralStack->CurrentWidget()->StopStreaming();
    session->LoadDefaults();
    centralStack->CurrentWidget()->StartStreaming();
}

// Get user input for a new name
// Replace the text in the .ini file
void MainWindow::RenamePreset(int p)
{
    bool ok;
    QString newName =
            QInputDialog::getText(this,
                                  tr("Preset Renaming"),
                                  tr("New Name(Between 3-20 characters)"),
                                  QLineEdit::Normal,
                                  QString(),
                                  &ok);

    if(ok && (newName.length() > 2) && (newName.length() < 20)) {
        QSettings settings(QSettings::IniFormat,
                           QSettings::UserScope,
                           "SignalHound",
                           "PresetNames");

        QString key;
        key.sprintf("PresetNames/Preset%d", p + 1);
        settings.setValue(key, newName);
    }
}

// Slot for when the main preset file menu is about to
//  be shown. All preset menu's share the same actions in
//  which the text for those actions are updated here with the
//  values found in the .ini file
void MainWindow::loadPresetNames()
{
    QSettings settings(QSettings::IniFormat,
                       QSettings::UserScope,
                       "SignalHound",
                       "PresetNames");

    QString key;
    QVariant value;
    QList<QAction*> saveActions = preset_save->actions();
    QList<QAction*> loadAction = preset_load->actions();
    QList<QAction*> renameActions = preset_name->actions();

    for(int i = 0; i < PRESET_COUNT; i++) {
        key.sprintf("Preset%d", i + 1);
        value = settings.value("PresetNames/" + key);

        if(value.isValid()) {
            saveActions.at(i)->setText(value.toString());
            loadAction.at(i)->setText(value.toString());
            renameActions.at(i)->setText(value.toString());
        } else {
            saveActions.at(i)->setText(key);
            loadAction.at(i)->setText(key);
            renameActions.at(i)->setText(key);
        }
    }
}

void MainWindow::loadPreset(QAction *a)
{
    centralStack->CurrentWidget()->StopStreaming();

    session->LoadPreset(a->data().toInt());
    int newMode = session->sweep_settings->Mode();

    newMode = ChangeMode((OperationalMode)newMode);

    centralStack->CurrentWidget()->changeMode(newMode);
}

void MainWindow::aboutToShowPresetMenu()
{
    preset_load->setEnabled(session->device->IsOpen());
    preset_save->setEnabled(session->device->IsOpen());
}

void MainWindow::modeChanged(QAction *a)
{
    centralStack->CurrentWidget()->StopStreaming();

    OperationalMode newMode = (OperationalMode)a->data().toInt();

    newMode = ChangeMode((OperationalMode)newMode);

    centralStack->CurrentWidget()->changeMode(newMode);
}

OperationalMode MainWindow::ChangeMode(OperationalMode newMode)
{
    centralStack->CurrentWidget()->EnableToolBarActions(false);

    // Ensure the TG is connected before switching into network analysis
    // If it is not, then go to previous mode
    if(newMode == MODE_NETWORK_ANALYZER) {
        if(!session->device->AttachTg()) {
            newMode = MODE_SWEEPING; //session->sweep_settings->Mode();
            session->sweep_settings->setMode(newMode);
            QMessageBox::warning(0, "Tracking Generator Not Found",
                                 "Tracking Generator Not Found\n"
                                 "Please ensure tracking generator is connected to your PC");
        }
    }

    sweep_panel->setMode(newMode);
    measure_panel->setMode(newMode);

    if(newMode == MODE_ZERO_SPAN) {
        centralStack->setCurrentWidget(demodCentral);
        sweep_panel->hide();
        measure_panel->hide();
        demodPanel->show();
    } else if(newMode == MODE_HARMONICS) {
        centralStack->setCurrentWidget(harmonicCentral);
        sweep_panel->show();
        measure_panel->hide();
        demodPanel->hide();
    } else if(newMode == MODE_NETWORK_ANALYZER) {
        centralStack->setCurrentWidget(tgCentral);
        sweep_panel->show();
        measure_panel->show();
        demodPanel->hide();
    } else if(newMode == MODE_PHASE_NOISE) {
        centralStack->setCurrentWidget(phaseNoiseCentral);
        sweep_panel->show();
        measure_panel->show();
        demodPanel->hide();
    } else {
        centralStack->setCurrentWidget(sweepCentral);
        demodPanel->hide();
        sweep_panel->show();
        measure_panel->show();
    }

    centralStack->CurrentWidget()->EnableToolBarActions(true);

    return newMode;
}

// Function gets called when the zero-span button is pressed on the sweep panel
// Know for a fact that the device is coming from a sweep mode
void MainWindow::zeroSpanPressed()
{
    centralStack->CurrentWidget()->StopStreaming();

    // Set the zero-span center to the current sweep center
    Frequency currentCenter = session->sweep_settings->Center();
    session->demod_settings->setCenterFreq(currentCenter);

    ChangeMode(MODE_ZERO_SPAN);
    centralStack->CurrentWidget()->changeMode(MODE_ZERO_SPAN);
}

void MainWindow::startAudioPlayer()
{
    int temp_mode = session->sweep_settings->Mode();

    centralStack->CurrentWidget()->changeMode(MODE_IDLE);
    // Start the Audio Dialog with the active center frequency
    session->audio_settings->setCenterFrequency(
                centralStack->CurrentWidget()->GetCurrentCenterFreq());

    AudioDialog *dlg = new AudioDialog(session->device, session->audio_settings);

    dlg->exec();
    //*session->audio_settings = *dlg->Configuration();
    delete dlg;

    centralStack->CurrentWidget()->changeMode(temp_mode);
}

void MainWindow::startMeasuringReceiever()
{
    int temp_mode = session->sweep_settings->Mode();

    centralStack->CurrentWidget()->changeMode(MODE_IDLE);
    double center = centralStack->CurrentWidget()->GetCurrentCenterFreq();
    MeasuringReceiver *dlg = new MeasuringReceiver(session->device, center, this);
    dlg->exec();
    delete dlg;

    centralStack->CurrentWidget()->changeMode(temp_mode);
}

void MainWindow::startSA124IFOutput()
{
    int temp_mode = session->sweep_settings->Mode();

    centralStack->CurrentWidget()->changeMode(MODE_IDLE);

    IFOutputDialog *dlg = new IFOutputDialog(session->device, this);
    dlg->exec();
    delete dlg;

    centralStack->CurrentWidget()->changeMode(temp_mode);
}

void MainWindow::startSelfTest()
{
    int temp_mode = session->sweep_settings->Mode();

    centralStack->CurrentWidget()->changeMode(MODE_IDLE);

    SelfTestDialog *dlg = new SelfTestDialog(session->device, this);
    dlg->exec();
    delete dlg;

    centralStack->CurrentWidget()->changeMode(temp_mode);
}

void MainWindow::aboutToShowTimebaseMenu()
{
    for(QAction *a : timebase_menu->actions()) {
        a->setChecked(a->data().toInt() == session->device->TimebaseReference());
    }
}

void MainWindow::timebaseChanged(QAction *a)
{
    centralStack->CurrentWidget()->StopStreaming();

    int setMode = session->device->SetTimebase(a->data().toInt());

    auto actions = timebase_menu->actions();
    for(QAction *a : actions) {
        a->setChecked(setMode == a->data().toInt());
    }

    centralStack->CurrentWidget()->StartStreaming();
}

void MainWindow::showPreferencesDialog()
{
    PreferenceDialog prefDlg(session);
    prefDlg.exec();
}

// String for our "About" box
QChar trademark_char(short(174));
QChar copyright_char(short(169));
const QString about_string =
        QWidget::tr("Signal Hound") + trademark_char + QWidget::tr("\n") +
        QWidget::tr("Copyright ") + copyright_char + QWidget::tr(" 2015\n");
const QString gui_version = "Software Version "
        + QString(VER_PRODUCTVERSION_STR)
        + QString("\n");

void MainWindow::showAboutBox()
{
    QString bb_api_string = tr("BB API Version ") + tr(bbGetAPIVersion());
    QString sa_api_string = tr("SA API Version ") + tr(saGetAPIVersion());
    QMessageBox::about(this, tr("About"),
                       about_string
                       + gui_version
                       + bb_api_string
                       + "\n"
                       + sa_api_string);
}
