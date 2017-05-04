#ifndef AUDIO_PANEL_H
#define AUDIO_PANEL_H

#include "dock_panel.h"
#include "entry_widgets.h"

class AudioSettings;

class AudioPanel : public DockPanel {
    Q_OBJECT

public:
    AudioPanel(const QString &title,
               QWidget *parent,
               AudioSettings *audio_settings);
    ~AudioPanel();

private:
    // Copy of settings ptr
    AudioSettings *settings_ptr;

    ComboEntry *mode_select;
    FrequencyEntry *if_bandwidth;
    FrequencyEntry *low_pass_freq;
    FrequencyEntry *high_pass_freq;
    // FM deemphasis

public slots:
    void update(const AudioSettings*);

signals:

private:
    DISALLOW_COPY_AND_ASSIGN(AudioPanel)
};

#endif // AUDIO_PANEL_H
