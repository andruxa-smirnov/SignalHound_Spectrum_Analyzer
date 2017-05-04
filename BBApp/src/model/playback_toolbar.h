#ifndef PLAYBACK_TOOLBAR_H
#define PLAYBACK_TOOLBAR_H

#include "lib/bb_lib.h"
#include "widgets/entry_widgets.h"
#include "session.h"
#include "sweep_settings.h"
#include "trace.h"

#include <QToolBar>
#include <QPushButton>
#include <QFile>
#include <QBuffer>

const unsigned short playback_signature = 0xBB60;
const unsigned short playback_version = 0x1;

// Version 1 header
struct playback_header {
    unsigned short signature;
    unsigned short version;

    int sweep_count;

    ushort title[MAX_TITLE_LEN + 1];
    double center_freq; // Sweep settings
    double span;
    double rbw;
    double vbw;
    double ref_level;
    double div;
    int atten;
    int gain;
    int detector;

    int trace_len;
    double trace_start_freq;
    double bin_size;
};

class PlaybackFile : public QObject {
    Q_OBJECT

    friend class PlaybackToolBar;
    // First byte of data after the header
    static const qint64 data_start = sizeof(playback_header);
public:
    PlaybackFile();
    ~PlaybackFile();

    bool Recording() const { return is_recording; }
    bool Playing() const { return is_playing; }

    // Return true if at end of file
    // Should only be called when playing back a file
    bool AtEndOfFile() const;

    void GetSweepConfig(SweepSettings *ss, QString &title);
    bool GetSweep(Trace *trace);

    int GetTracePos() const { return trace_pos; }
    int GetFileSize() const { return header.sweep_count; }

    qint64 GetFilePosition() const { return file_handle.pos(); }

    void SetTracePos(int pos);

    bool PutSweep(const Trace *trace);
    void PrepFirstTrace(const Trace *trace);
    void CloseFile();
    void CloseRecording();

private:
    playback_header header;
    qint64 trace_pos; // position in file
    qint64 step_size; // bytes per trace

    ulong timeout;

    QFile file_handle;
    std::mutex buffer_mutex;
    std::atomic<bool> is_recording;
    std::atomic<bool> is_playing;

    void stopRecording() { CloseRecording(); }

private slots:
    void startRecording();

    bool play();

signals:

private:
    DISALLOW_COPY_AND_ASSIGN(PlaybackFile)
};

// Toolbar / Recorder / Retriever for regular sweeps
// Works in sweep mode and real-time mode only
// Works directly with sweep_settings and trace classes

class PlaybackToolBar : public QToolBar {
    Q_OBJECT

    const Preferences *prefs;

public:
    PlaybackToolBar(const Preferences *preferences,
                    QWidget *parent = 0);
    ~PlaybackToolBar();

    void PutTrace(const Trace *t);
    bool GetTrace(Trace *t);
    void GetPlaybackSettings(SweepSettings *settings, QString &title) {
        if(file_io->Playing()) file_io->GetSweepConfig(settings, title);
    }

    void Stop() {
        if(file_io->Playing()) stopPlayingPressed();
        if(file_io->Recording()) stopRecordPressed();
    }

private:
    QPushButton *record_btn, *stop_record_btn;
    QPushButton *play_btn, *stop_play_btn, *pause_btn;
    QPushButton *rewind_btn, *step_back_btn, *step_fwd_btn;

    Label *trace_label; // Trace number over total traces
    Label *time_label; // Time of the last trace recieved
    Label *size_label;

    QSlider *trace_slider;

    PlaybackFile *file_io;
    SleepEvent timer;
    std::atomic<bool> paused;

public slots:

private slots:
    void recordPressed();
    void stopRecordPressed();

    void playPressed();
    void stopPlayingPressed();
    void pausePressed();
    void rewindPressed();
    void stepBackPressed();
    void stepForwardPressed();
    //void setDelayPressed();
    void sliderPosChanged(int);

    void showFileNameSaved();

signals:
    void startRecording(bool);
    void startPlaying(bool);

    void showFilenameInGuiThread();

private:
    DISALLOW_COPY_AND_ASSIGN(PlaybackToolBar)
};

#endif // PLAYBACK_TOOLBAR_H
