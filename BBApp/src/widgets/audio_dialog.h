#ifndef AUDIO_DIALOG_H
#define AUDIO_DIALOG_H

#include <thread>
#include <atomic>

#include <QDialog>
#include <QTimer>

#include "../model/device.h"
#include "../model/audio_settings.h"
#include "../widgets/dock_page.h"
#include "../widgets/entry_widgets.h"

class AudioDialog : public QDialog {
    Q_OBJECT

public:
    AudioDialog(Device *device_ptr,
                AudioSettings *settings_ptr);
    ~AudioDialog();

    const AudioSettings* Configuration() const { return config; }

protected:
    void keyPressEvent(QKeyEvent *e);

private:
    void Reconfigure();
    void AudioThread();

    Device *device; // Does not own
    AudioSettings *config; // Does not own
    float *from_device;
    short *buffer;

    std::thread thread_handle;
    std::atomic<bool> running, update;

    DockPage *bandwidth_page, *frequency_page;
    FrequencyEntry *frequency_entry;
    ComboEntry *type_entry;
    FrequencyEntry *bandwidth_entry;
    FrequencyEntry *low_pass_entry;
    FrequencyEntry *high_pass_entry;
    NumericEntry *deemphasis;

    SHPushButton *smallDec, *smallInc;
    SHPushButton *largeDec, *largeInc;
    QTimer reset_timer;

    SHPushButton *okBtn;

    double low_limit, high_limit;
    double factor; // factor = log2(factor)
    int x_factor;

    void incrementFactor() { factor = log10(++x_factor) / log10(2.0); }
    void resetFactor() { factor = 1.0; x_factor = 2; }

private slots:
    void configChanged() { update = true; }

    void smallIncPressed();
    void smallDecPressed();
    void largeIncPressed();
    void largeDecPressed();
    void released() { resetFactor(); }

    // Set audioDlg focus after reconfigure
    void setAnonFocusSlot() { setFocus(); }

signals:
    void setAnonFocus();
};

#endif // AUDIO_DIALOG_H
