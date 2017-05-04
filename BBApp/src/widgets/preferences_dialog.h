#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <QDialog>
#include <QRadioButton>
#include <QGroupBox>
#include <QButtonGroup>

#include "../widgets/dock_page.h"
#include "../widgets/dock_panel.h"
#include "../model/session.h"

const int PREF_RADIO_WIDTH = 300;
const int PREF_PAGE_WIDTH = 400;
const int PREF_DLG_HEIGHT = 400;

// Single pref panel
// The preferences dialog contains many pref panels
class PreferencePanel : public QScrollArea {
    Q_OBJECT

public:
    PreferencePanel(const QString &panelTitle, QWidget *parent = 0);
    ~PreferencePanel();

    QString GetTitle() const { return title; }
    void AddPage(DockPage *page);

    // Use the session to populate the entry widgets
    virtual void Populate(const Session *session) = 0;
    // Called when apply is pressed, updates the session
    //   with the current entry widget values
    virtual void Apply(Session *session) = 0;

protected:
    void resizeEvent(QResizeEvent *);

    QString title;

public slots:
    void tabsChanged();

private:
    QWidget *scrollWidget;
    QVector<DockPage*> tabs;

private:
    DISALLOW_COPY_AND_ASSIGN(PreferencePanel)
};

// Page for editing all colors except the trace colors
class PreferenceColorPanel : public PreferencePanel {
    Q_OBJECT

public:
    PreferenceColorPanel(const QString &panelTitle,
                         QWidget *parent = 0);
    ~PreferenceColorPanel();

    void Populate(const Session *session);
    void Apply(Session *session);

private:
    // View Page
    NumericEntry *traceWidth, *graticuleWidth;
    CheckBoxEntry *graticuleStipple;
    // Color Page
    ColorEntry *background, *text, *graticule;
    ColorEntry *markerBorder, *markerBackground, *markerText;
    ColorEntry *limitLines;
    // Sweep Settings
    NumericEntry *sweepDelay;
    NumericEntry *realTimeSweepTime;
    // Playback Settings
    NumericEntry *playbackDelay;
    NumericEntry *maxSaveFileSize;

private:
    DISALLOW_COPY_AND_ASSIGN(PreferenceColorPanel)
};

// Trace preferences panel
class PreferenceTracePanel : public PreferencePanel {
    Q_OBJECT

public:
    PreferenceTracePanel(const QString &panelTitle,
                         QWidget *parent = 0);
    ~PreferenceTracePanel();

    void Populate(const Session *session);
    void Apply(Session *session);

private:


private:
    DISALLOW_COPY_AND_ASSIGN(PreferenceTracePanel)
};

class PreferenceDevicePanel : public PreferencePanel {
    Q_OBJECT

public:
    PreferenceDevicePanel(const QString &panelTitle,
                          QWidget *parent = 0);
    ~PreferenceDevicePanel();

    void Populate(const Session *session);
    void Apply(Session *session);

private:

private:
    DISALLOW_COPY_AND_ASSIGN(PreferenceDevicePanel)
};

// Full preferences dialog
// Tabbed panels
class PreferenceDialog : public QDialog {
    Q_OBJECT

public:
    PreferenceDialog(Session *session, QWidget *parent = 0);
    ~PreferenceDialog();

private:
    // Does not own
    Session *session_ptr;

    // One panel per radio button
    // Index of selected radio button determines which panel is
    QButtonGroup *radioGroup;
    QGroupBox *groupBox;
    // List of radio buttons, correspond to panels
    QVector<PreferencePanel*> panels;
    SHPushButton *okBtn, *cancelBtn, *applyBtn;

private slots:
    void switchPanel(int panelIndex);
    void applyClicked();

private:
    DISALLOW_COPY_AND_ASSIGN(PreferenceDialog)
};

#endif // PREFERENCES_DIALOG_H
