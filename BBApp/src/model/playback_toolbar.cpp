#include "playback_toolbar.h"

#include <QFileDialog>
#include <QLayout>
#include <QIcon>
#include <QMessageBox>

PlaybackFile::PlaybackFile()
{
    timeout = 0;
    trace_pos = 0;
    is_recording = false;
}

PlaybackFile::~PlaybackFile()
{
    if(is_recording) {
        CloseRecording();
    }
}

bool PlaybackFile::AtEndOfFile() const
{
    if(Playing() && (GetTracePos() >= GetFileSize())) {
        return true;
    }

    return false;
}

void PlaybackFile::GetSweepConfig(SweepSettings *ss, QString &title)
{
    title = QString().fromUtf16(header.title);

    ss->setCenter(header.center_freq);
    ss->setSpan(header.span);
    ss->setRBW(header.rbw);
    ss->setVBW(header.vbw);
    ss->setRefLevel(header.ref_level);
    ss->setDiv(header.div);
    ss->setAttenuation(header.atten);
    ss->setGain(header.gain);
    ss->setDetector(header.detector);
}

bool PlaybackFile::GetSweep(Trace *trace)
{
    qint64 time;
    if(!is_playing || !file_handle.isOpen()) {
        return false;
    }

    if(trace_pos >= header.sweep_count) {
        return true;
    }

    //std::lock_guard<std::mutex> lg(buffer_mutex);

    trace->SetSize(header.trace_len);
    trace->SetUpdateRange(0, trace->Length());
    trace->SetFreq(header.bin_size, header.trace_start_freq);

    file_handle.seek(data_start + step_size * trace_pos);

    file_handle.read((char*)&time, sizeof(qint64));
    trace->SetTime(time);
    file_handle.read((char*)trace->Min(), sizeof(float) * header.trace_len);
    file_handle.read((char*)trace->Max(), sizeof(float) * header.trace_len);

    trace_pos++;

    return true;
}

void PlaybackFile::SetTracePos(int pos)
{
    if(!is_playing) return;

    if(pos < 0) pos = 0;
    if(pos > header.sweep_count) pos = header.sweep_count;

    trace_pos = pos;
}

bool PlaybackFile::PutSweep(const Trace *trace)
{
    if(!is_recording) {
        return false;
    }

    std::lock_guard<std::mutex> lg(buffer_mutex);

    if(trace_pos == 0) {
        PrepFirstTrace(trace);
        file_handle.seek(data_start);
    }

    qint64 current_ms = bb_lib::get_ms_since_epoch();
    file_handle.write((char*)&current_ms, sizeof(qint64));
    file_handle.write((char*)trace->Min(), sizeof(float) * trace->Length());
    file_handle.write((char*)trace->Max(), sizeof(float) * trace->Length());

    trace_pos++;

    return true;
}

void PlaybackFile::PrepFirstTrace(const Trace *trace)
{
    const SweepSettings *set = trace->GetSettings();

    bb_lib::cpy_16u(Session::GetTitle().utf16(),
                    header.title, MAX_TITLE_LEN);

    header.signature = playback_signature;
    header.version = playback_version;

    header.center_freq = set->Center();
    header.span = set->Span();
    header.rbw = set->RBW();
    header.vbw = set->VBW();
    header.ref_level = set->RefLevel();
    header.div = set->Div();
    header.atten = set->Atten();
    header.gain = set->Gain();
    header.detector = set->Detector();

    header.trace_len = trace->Length();
    header.trace_start_freq = trace->StartFreq();
    header.bin_size = trace->BinSize();

    step_size = sizeof(qint64) + 2.0 * trace->Length() * sizeof(float);
}

void PlaybackFile::CloseFile()
{
    is_playing = false;
    trace_pos = 0;

    file_handle.close();
}

void PlaybackFile::CloseRecording()
{
    is_recording = false;

    std::lock_guard<std::mutex> lg(buffer_mutex);

    if(trace_pos == 0) {
        return;
    }

    header.sweep_count = trace_pos;

    file_handle.seek(0);
    file_handle.write((char*)&header, sizeof(playback_header));
    file_handle.close();

    //QMessageBox::information(0, tr("File Saved"), tr("Recording saved at ") + file_handle.fileName());
}

void PlaybackFile::startRecording()
{
    if(file_handle.isOpen()) return;

    QString file_name = bb_lib::get_my_documents_path() +
            bb_lib::get_recording_filename();

    //file_name = "some_file.sweep";

    file_handle.setFileName(file_name);
    file_handle.open(QIODevice::WriteOnly);

    // Handle file not opening here

    trace_pos = 0;
    is_recording = true;
}

bool PlaybackFile::play()
{
    if(is_playing) {
        return false;
    }

    QString file_name = QFileDialog::getOpenFileName(0, tr("Playback a recorded session"),
                                                     bb_lib::get_my_documents_path(),
                                                     tr("Sweep Files (*.bbr)"));
    if(file_name.isNull()) return false;

    file_handle.setFileName(file_name);
    file_handle.open(QIODevice::ReadOnly);
    if(!file_handle.isOpen()) {
        return false;
    } else {
        trace_pos = 0;
        is_playing = true;
    }

    // Get header, check signature and version
    file_handle.read((char*)&header, sizeof(playback_header));

    if(header.signature != playback_signature) {
        QMessageBox::warning(0, tr("Invalid File"), tr("Unable to recognize playback file"));
        return false;
    }

    if(header.version != playback_version) {
        QMessageBox::warning(0, tr("Unknown file version"), tr("Unrecognized file version"));
        return false;
    }

    step_size = 2.0 * sizeof(float) * header.trace_len + sizeof(qint64);

    return true;
}

PlaybackToolBar::PlaybackToolBar(const Preferences *preferences,
                                 QWidget *parent) :
    QToolBar(parent),
    prefs(preferences)
{
    file_io = new PlaybackFile();

    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);

    trace_label = new Label(tr("Inactive"), this);
    trace_label->setFixedSize(70, 32);
    trace_label->setAlignment(Qt::AlignCenter);
    addWidget(trace_label);

    addSeparator();

    record_btn = new QPushButton(QIcon(":/playback/record.png"), "", this);
    record_btn->setObjectName("BBFlatButton");
    record_btn->setFixedSize(32, 32);
    connect(record_btn, SIGNAL(clicked()), this, SLOT(recordPressed()));
    addWidget(record_btn);

    stop_record_btn = new QPushButton(QIcon(":/playback/stop.png"), "", this);
    stop_record_btn->setObjectName("BBFlatButton");
    stop_record_btn->setFixedSize(32, 32);
    connect(stop_record_btn, SIGNAL(clicked()), this, SLOT(stopRecordPressed()));
    addWidget(stop_record_btn);

    addSeparator();

    play_btn = new QPushButton(QIcon(":/playback/play.png"), "", this);
    play_btn->setObjectName("BBFlatButton");
    play_btn->setFixedSize(32, 32);
    connect(play_btn, SIGNAL(clicked()), this, SLOT(playPressed()));
    addWidget(play_btn);

    stop_play_btn = new QPushButton(QIcon(":/playback/stop.png"), "", this);
    stop_play_btn->setObjectName("BBFlatButton");
    stop_play_btn->setFixedSize(32, 32);
    connect(stop_play_btn, SIGNAL(clicked()), this, SLOT(stopPlayingPressed()));
    addWidget(stop_play_btn);

    pause_btn = new QPushButton(QIcon(":/playback/pause.png"), "", this);
    pause_btn->setObjectName("BBFlatButton");
    pause_btn->setFixedSize(32, 32);
    connect(pause_btn, SIGNAL(clicked()), this, SLOT(pausePressed()));
    addWidget(pause_btn);

    rewind_btn = new QPushButton(QIcon(":/playback/rewind.png"), "", this);
    rewind_btn->setObjectName("BBFlatButton");
    rewind_btn->setFixedSize(32, 32);
    connect(rewind_btn, SIGNAL(clicked()), this, SLOT(rewindPressed()));
    addWidget(rewind_btn);

    step_back_btn = new QPushButton(QIcon(":/playback/skip_previous.png"), "", this);
    step_back_btn->setObjectName("BBFlatButton");
    step_back_btn->setFixedSize(32, 32);
    connect(step_back_btn, SIGNAL(clicked()), this, SLOT(stepBackPressed()));
    addWidget(step_back_btn);

    step_fwd_btn = new QPushButton(QIcon(":/playback/skip_next.png"), "", this);
    step_fwd_btn->setObjectName("BBFlatButton");
    step_fwd_btn->setFixedSize(32, 32);
    connect(step_fwd_btn, SIGNAL(clicked()), this, SLOT(stepForwardPressed()));
    addWidget(step_fwd_btn);

//    timer_btn = new QPushButton(QIcon(":/playback/time.png"), "", this);
//    timer_btn->setObjectName("BBFlatButton");
//    timer_btn->setFixedSize(32, 32);
//    connect(timer_btn, SIGNAL(clicked()), this, SLOT(setDelayPressed()));
//    addWidget(timer_btn);

    addSeparator();

    time_label = new Label("", this);
    time_label->setFixedSize(200, 32);
    time_label->setAlignment(Qt::AlignCenter);
    addWidget(time_label);

    trace_slider = new QSlider(Qt::Horizontal, this);
    trace_slider->setFixedHeight(32);
    trace_slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    trace_slider->setTracking(false);
    trace_slider->setEnabled(false);
    trace_slider->setRange(0, 0);
    addWidget(trace_slider);
    connect(trace_slider, SIGNAL(sliderPressed()),
            this, SLOT(pausePressed()));
    connect(trace_slider, SIGNAL(valueChanged(int)),
            this, SLOT(sliderPosChanged(int)));
    connect(trace_slider, SIGNAL(sliderReleased()),
            this, SLOT(playPressed()));

    size_label = new Label("", this);
    size_label->setFixedSize(150, 32);
    size_label->setAlignment(Qt::AlignCenter);
    addWidget(size_label);

    //QWidget *spacer = new QWidget();
    //spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //addWidget(spacer);

    connect(this, SIGNAL(startRecording(bool)), record_btn, SLOT(setDisabled(bool)));    
    connect(this, SIGNAL(startRecording(bool)), stop_record_btn, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(startRecording(bool)), play_btn, SLOT(setDisabled(bool)));

    connect(this, SIGNAL(startPlaying(bool)), record_btn, SLOT(setDisabled(bool)));

    //connect(this, SIGNAL(startPlaying(bool)), play_btn, SLOT(setDisabled(bool)));
    connect(this, SIGNAL(startPlaying(bool)), stop_play_btn, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(startPlaying(bool)), pause_btn, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(startPlaying(bool)), rewind_btn, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(startPlaying(bool)), step_back_btn, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(startPlaying(bool)), step_fwd_btn, SLOT(setEnabled(bool)));

    connect(this, SIGNAL(showFilenameInGuiThread()),
            this, SLOT(showFileNameSaved()));

    emit startPlaying(false);
    emit startRecording(false);

    //connect(play_btn, SIGNAL(clicked()), file_io, SLOT(play()));
}

PlaybackToolBar::~PlaybackToolBar()
{
    file_io->CloseFile();
    timer.Wake(); // Break any waiting threads
    delete file_io;
}

void PlaybackToolBar::PutTrace(const Trace *t)
{
    if(file_io->PutSweep(t)) {
        QString fileSizeStr;
        qint64 fileSize = file_io->GetFilePosition();

        if(fileSize >= (qint64(1.0e9) * qint64(prefs->playbackMaxFileSize))) {
            stopRecordPressed();
            //stop_record_btn->click();
        } else {
            fileSizeStr.sprintf(" %f GB", (double)fileSize / 1.0e9);
            size_label->setText(fileSizeStr);
        }
    }
}

bool PlaybackToolBar::GetTrace(Trace *t)
{
//    if(file_io->AtEndOfFile()) {
//        timer.Sleep(32);
//        return true;
//    }

    if(paused || file_io->AtEndOfFile()) {
        timer.Sleep();
        //return true;
    } else {
        timer.Sleep(prefs->playbackDelay);
    }

    if(file_io->GetSweep(t)) {
        time_label->setText(bb_lib::get_time_string(t->Time()));
        trace_slider->setSliderPosition(file_io->GetTracePos());

        QString overall_count;
        overall_count.sprintf(" %d / %d", file_io->GetTracePos(), file_io->GetFileSize());
        size_label->setText(overall_count);

        return true;
    }

    return false;
}

void PlaybackToolBar::recordPressed()
{
    trace_label->setText("Recording");

    file_io->startRecording();

    emit startRecording(true);
}

void PlaybackToolBar::stopRecordPressed()
{
    trace_label->setText("Inactive");
    size_label->setText("");
    file_io->stopRecording();

    emit showFilenameInGuiThread();
    emit startRecording(false);
}

void PlaybackToolBar::playPressed()
{
    if(file_io->Playing()) {
        paused = false;
        timer.Wake();
        return;
    }

    // Try to open a file for playing
    if(!file_io->play()) {
        return;
    }

    trace_label->setText("Playing");
    trace_slider->setEnabled(true);
    trace_slider->setRange(0, file_io->GetFileSize());
    trace_slider->setPageStep(file_io->GetFileSize() / 10);

    paused = false;
    timer.Wake();

    emit startPlaying(true);
}

void PlaybackToolBar::stopPlayingPressed()
{
    file_io->CloseFile();

    trace_label->setText("Inactive");
    time_label->setText("");
    size_label->setText("");
    trace_slider->setMinimum(0);
    trace_slider->setMaximum(0);
    //trace_slider->setEnabled(false);
    //trace_slider->setRange(0, 0);

    // Wake a paused thread after the file_io has closed
    timer.Wake();

    emit startPlaying(false);
}

void PlaybackToolBar::pausePressed()
{
    paused = true;
    if(!paused)
        timer.Wake();
}

void PlaybackToolBar::rewindPressed()
{
    paused = true;
    file_io->SetTracePos(0);
    timer.Wake();
}

void PlaybackToolBar::stepBackPressed()
{
    paused = true;
    file_io->SetTracePos(file_io->GetTracePos() - 2);
    timer.Wake();
}

void PlaybackToolBar::stepForwardPressed()
{
    paused = true;
    timer.Wake();
}

//void PlaybackToolBar::setDelayPressed()
//{

//}

void PlaybackToolBar::sliderPosChanged(int new_pos)
{
    file_io->SetTracePos(new_pos);
    timer.Wake();
}

void PlaybackToolBar::showFileNameSaved()
{
    QMessageBox::information(0, tr("File Saved"),
                             tr("Recording saved at ") +
                             file_io->file_handle.fileName());
}


