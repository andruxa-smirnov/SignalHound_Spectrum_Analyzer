#include "audio_panel.h"
#include "../model/audio_settings.h"

AudioPanel::AudioPanel(const QString &title,
                       QWidget *parent,
                       AudioSettings *audio_settings)
    : DockPanel(title, parent), settings_ptr(audio_settings)
{
    DockPage *settings_page = new DockPage("Settings");

    QStringList combo_text;

    mode_select = new ComboEntry("Audio Type");
    combo_text << "AM" << "FM" << "Upper side band"
               << "Lower Side Band" << "CW";
    mode_select->setComboText(combo_text);
    combo_text.clear();

    if_bandwidth = new FrequencyEntry("IF Bandwidth",
                                      settings_ptr->IFBandwidth());
    low_pass_freq = new FrequencyEntry("Low Pass Freq",
                                       settings_ptr->LowPassFreq());
    high_pass_freq = new FrequencyEntry("High Pass Freq",
                                        settings_ptr->HighPassFreq());

    settings_page->AddWidget(mode_select);
    settings_page->AddWidget(if_bandwidth);
    settings_page->AddWidget(low_pass_freq);
    settings_page->AddWidget(high_pass_freq);

    AddPage(settings_page);

    update(settings_ptr);
}

AudioPanel::~AudioPanel()
{

}

void AudioPanel::update(const AudioSettings *settings)
{
    mode_select->setComboIndex(settings->AudioMode());
    if_bandwidth->SetFrequency(settings->IFBandwidth());
    low_pass_freq->SetFrequency(settings->LowPassFreq());
    high_pass_freq->SetFrequency(settings->HighPassFreq());
}
