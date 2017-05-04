#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStatusBar>
#include <QAction>

#include "model/session.h"
#include "widgets/measure_panel.h"
#include "widgets/sweep_panel.h"
#include "widgets/demod_panel.h"
#include "widgets/tg_panel.h"
#include "widgets/status_bar.h"
#include "widgets/progress_dialog.h"
#include "views/central_stack.h"
#include "views/sweep_central.h"
#include "views/demod_central.h"
#include "views/harmonics_central.h"
#include "views/tg_central.h"
#include "views/phase_noise_central.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    Session* GetSession() { return session; }
    static BBStatusBar* GetStatusBar() { return status_bar; }

protected:
    void closeEvent(QCloseEvent *);

private:
    void InitMenuBar();
    void SaveState();
    void RestoreState();
    // Change centralWidget, toolBars, dockWidgets
    // Return the mode that was set
    OperationalMode ChangeMode(OperationalMode newMode);

    void RenamePreset(int p);

    void OpenDevice(QMap<QString, QVariant> devInfoMap);
    void OpenDeviceInThread(QEventLoop *el, Device *device, int serialToOpen);
    void Preset();
    void PresetDeviceInThread(QEventLoop *el);

    // The one and only session instance
    //   all other Session pointers are copies
    Session *session;

    QMenuBar *main_menu;
    QMenu *file_menu, *connect_menu;
    QMenu *preset_menu;
    QMenu *preset_load;
    QMenu *preset_save;
    QMenu *preset_name;
    QMenu *mode_menu;
    QMenu *settings_menu, *timebase_menu;
    QMenu *utilities_menu;
    QMenu *help_menu;

    SweepPanel *sweep_panel;
    MeasurePanel *measure_panel;
    DemodPanel *demodPanel;
    TgCtrlPanel *tgPanel;

    CentralStack *centralStack;
    SweepCentral *sweepCentral;
    DemodCentral *demodCentral;
    HarmonicsCentral *harmonicCentral;
    TGCentral *tgCentral;
    PhaseNoiseCentral *phaseNoiseCentral;

    static BBStatusBar *status_bar;

    // Used for opening/closing BB60
    //std::thread device_thread;
    //ProgressDialog progressDialog;
    bool saveLayoutOnClose;

public slots:
    void connectDeviceUponOpen();
    void populateConnectMenu();
    void connectDevice(QAction *a);
    // Call direct when user closes device
    void disconnectDevice();
    // Call when device must be forced close
    void forceDisconnectDevice();
    void presetDevice();

private slots:
    void deviceConnected(bool);

    // Restore the default layout for control panels
    void restoreDefaultLayout();

    void aboutToShowFileMenu();
    void aboutToShowSettingsMenu();
    void aboutToShowModeMenu();
    void aboutToShowUtilitiesMenu();

    void printView();
    void saveAsImage();

    void setTitle();
    void clearTitle() { session->SetTitle(QString()); }
    void loadDefaultColors();
    void loadPrinterFriendlyColors();
    void saveAsDefaultColorScheme();
    void loadStyleLight() { session->prefs.SetProgramStyle(LIGHT_STYLE_SHEET); }
    void loadStyleDark() { session->prefs.SetProgramStyle(DARK_STYLE_SHEET); }
    void loadStyleBlue() { session->prefs.SetProgramStyle(BLUE_STYLE_SHEET); }

    void loadDefaultSettings();
    void loadPreset(QAction *a);
    void aboutToShowPresetMenu();
    void savePreset(QAction *a) { session->SavePreset(a->data().toInt()); }
    void renamePreset(QAction *a) { RenamePreset(a->data().toInt()); }
    void loadPresetNames();
    void modeChanged(QAction *a);
    void zeroSpanPressed();
    void startAudioPlayer();
    void startMeasuringReceiever();
    void startSA124IFOutput();
    void startSelfTest();
    void aboutToShowTimebaseMenu();
    void timebaseChanged(QAction *a);
    void showPreferencesDialog();
    void showAboutBox();

signals:

private:
    DISALLOW_COPY_AND_ASSIGN(MainWindow)
};

#endif // MAINWINDOW_H
