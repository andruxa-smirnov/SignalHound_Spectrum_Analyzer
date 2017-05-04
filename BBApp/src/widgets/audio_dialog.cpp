#include "audio_dialog.h"

#include <QGroupBox>
#include <QShortcut>
#include <QKeyEvent>

#include <windows.h>
#pragma comment(lib,"Winmm.lib")

// Audio related
#define BLOCK_SIZE  8192
#define BLOCK_COUNT 20

// WaveOut function prototypes
static void CALLBACK waveOutProc(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
static WAVEHDR* allocateBlocks(int size, int count);
static void freeBlocks(WAVEHDR* blockArray);
static void writeAudio(HWAVEOUT hWaveOut, LPSTR data, int size);

// WaveOut variables
static CRITICAL_SECTION waveCriticalSection;
static WAVEHDR* waveBlocks;
volatile int waveFreeBlockCount;
static int waveCurrentBlock;

AudioDialog::AudioDialog(Device *device_ptr,
                         AudioSettings *settings_ptr) :
    device(device_ptr),
    config(settings_ptr),
    running(true),
    update(false),
    low_limit(0.0),
    high_limit(0.0),
    factor(1.0),
    x_factor(0)
{
    setWindowTitle("Audio Player");
    setObjectName("SH_Page");
    setSizeGripEnabled(false);

    bandwidth_page = new DockPage("Audio Configuration", this);
    frequency_page = new DockPage("Frequency Configuration", this);

    type_entry = new ComboEntry("Audio Type", this);
    QStringList types;
    // Indices must match API header defines
    types << "AM" << "FM" << "USB" << "LSB" << "CW";
    type_entry->setComboText(types);
    type_entry->setComboIndex(config->AudioMode());

    frequency_entry = new FrequencyEntry("Center Freq", config->CenterFreq());
    bandwidth_entry = new FrequencyEntry("Bandwidth", config->IFBandwidth());
    low_pass_entry = new FrequencyEntry("Low Pass", config->LowPassFreq());
    high_pass_entry = new FrequencyEntry("High Pass", config->HighPassFreq());
    deemphasis = new NumericEntry("Deemphasis", config->FMDeemphasis(), "us");

    frequency_page->AddWidget(frequency_entry);

    bandwidth_page->AddWidget(type_entry);
    bandwidth_page->AddWidget(bandwidth_entry);
    bandwidth_page->AddWidget(low_pass_entry);
    bandwidth_page->AddWidget(high_pass_entry);
    bandwidth_page->AddWidget(deemphasis);

    frequency_page->move(0, 0);
    bandwidth_page->move(MAX_DOCK_WIDTH, 0);

    smallInc = new SHPushButton("+20 Hz", this);
    smallInc->move(100, 75);
    smallInc->resize(100, 25);
    smallInc->setAutoRepeat(true);
    smallInc->setAutoRepeatDelay(300);
    smallInc->setAutoRepeatInterval(100);
    smallInc->setShortcut(QKeySequence(Qt::Key_Up));
    smallDec = new SHPushButton("-20 Hz", this);
    smallDec->move(100, 125);
    smallDec->resize(100, 25);
    smallDec->setAutoRepeat(true);
    smallDec->setAutoRepeatDelay(300);
    smallDec->setAutoRepeatInterval(100);
    smallDec->setShortcut(QKeySequence(Qt::Key_Down));
    largeDec = new SHPushButton("-1 kHz", this);
    largeDec->move(50, 100);
    largeDec->resize(100, 25);
    largeDec->setAutoRepeat(true);
    largeDec->setAutoRepeatDelay(300);
    largeDec->setAutoRepeatInterval(100);
    largeDec->setShortcut(QKeySequence(Qt::Key_Left));
    largeInc = new SHPushButton("+1 kHz", this);
    largeInc->move(150, 100);
    largeInc->resize(100, 25);
    largeInc->setAutoRepeat(true);
    largeInc->setAutoRepeatDelay(300);
    largeInc->setAutoRepeatInterval(100);
    largeInc->setShortcut(QKeySequence(Qt::Key_Right));

    setFixedSize(MAX_DOCK_WIDTH * 2, bandwidth_page->GetTotalHeight() + 30);

    okBtn = new SHPushButton(tr("OK"), this);
    okBtn->resize(90, 30);
    okBtn->move(width() - 95, height() - 35);
    connect(okBtn, SIGNAL(clicked()), this, SLOT(accept()));

    connect(type_entry, SIGNAL(comboIndexChanged(int)),
            config, SLOT(setMode(int)));
    connect(frequency_entry, SIGNAL(freqViewChanged(Frequency)),
            config, SLOT(setCenterFrequency(Frequency)));
    connect(bandwidth_entry, SIGNAL(freqViewChanged(Frequency)),
            config, SLOT(setIFBandwidth(Frequency)));
    connect(low_pass_entry, SIGNAL(freqViewChanged(Frequency)),
            config, SLOT(setLowPassFreq(Frequency)));
    connect(high_pass_entry, SIGNAL(freqViewChanged(Frequency)),
            config, SLOT(setHighPassFreq(Frequency)));
    connect(deemphasis, SIGNAL(valueChanged(double)),
            config, SLOT(setFMDeemphasis(double)));

    connect(config, SIGNAL(updated(const AudioSettings*)),
            this, SLOT(configChanged()));

    connect(smallInc, SIGNAL(clicked()), SLOT(smallIncPressed()));
    connect(smallDec, SIGNAL(clicked()), SLOT(smallDecPressed()));
    connect(largeInc, SIGNAL(clicked()), SLOT(largeIncPressed()));
    connect(largeDec, SIGNAL(clicked()), SLOT(largeDecPressed()));

    from_device = new float[4096];
    buffer = new short[4096];

    reset_timer.setInterval(500);
    connect(&reset_timer, SIGNAL(timeout()), this, SLOT(released()));

    connect(this, SIGNAL(setAnonFocus()), this, SLOT(setAnonFocusSlot()));

    thread_handle = std::thread(&AudioDialog::AudioThread, this);
}

AudioDialog::~AudioDialog()
{
    running = false;
    if(thread_handle.joinable()) {
        thread_handle.join();
    }

    delete [] from_device;
    delete [] buffer;
}

void AudioDialog::keyPressEvent(QKeyEvent *e)
{
    if(e->key() != Qt::Key_Enter) {
        QWidget::keyPressEvent(e);
    }
}

void AudioDialog::Reconfigure()
{
    device->ConfigureAudio(*config);

    frequency_entry->SetFrequency(config->CenterFreq());
    type_entry->setComboIndex(config->AudioMode());
    bandwidth_entry->SetFrequency(config->IFBandwidth());
    low_pass_entry->SetFrequency(config->LowPassFreq());
    high_pass_entry->SetFrequency(config->HighPassFreq());
    deemphasis->SetValue(config->FMDeemphasis());

    emit setAnonFocus();
}

void AudioDialog::AudioThread()
{
    // Initialize waveOut
    HWAVEOUT hWaveOut; // device handle
    WAVEFORMATEX wfx;

    waveBlocks = allocateBlocks(BLOCK_SIZE, BLOCK_COUNT);
    waveFreeBlockCount = BLOCK_COUNT;
    waveCurrentBlock = 0;
    InitializeCriticalSection(&waveCriticalSection);

    // set up the WAVEFORMATEX structure.
    wfx.nSamplesPerSec = device_traits::audio_rate(); // 32000;
    wfx.wBitsPerSample = 16;         // sample size
    wfx.nChannels = 1;               // channels
    wfx.cbSize = 0;                  // size of _extra_ info
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nBlockAlign = (wfx.wBitsPerSample*wfx.nChannels) >> 3;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign*wfx.nSamplesPerSec;

    // Try to open the default wave device. WAVE_MAPPER is
    //   a constant defined in mmsystem.h, it always points to the
    //   default wave device on the system (some people have 2 or
    //   more sound cards).
    if( waveOutOpen(
        &hWaveOut,
        WAVE_MAPPER,
        &wfx,
        (DWORD_PTR)waveOutProc,
        (DWORD_PTR)&waveFreeBlockCount,
        CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
    {
        //BB60Error("Error opening sound card");
        return;
    }

    Reconfigure();

    // Main loop
    while(running) {

        if(update) {
            Reconfigure();
            update = false;
        }

        device->GetAudio(from_device);

        for(int i = 0; i < 4096; i++) {
            buffer[i] = from_device[i] * 16000.0;
        }

        writeAudio(hWaveOut, (char*)buffer, 8192);
    }

    return;
}

void AudioDialog::smallIncPressed()
{
    config->setCenterFrequency(config->CenterFreq() + 20.0 * factor);
    incrementFactor();
    reset_timer.stop();
    reset_timer.start();
}

void AudioDialog::smallDecPressed()
{
    config->setCenterFrequency(config->CenterFreq() - 20.0 * factor);
    incrementFactor();
    reset_timer.stop();
    reset_timer.start();
}

void AudioDialog::largeIncPressed()
{
    config->setCenterFrequency(config->CenterFreq() + 1.0e3 * factor);
    incrementFactor();
    reset_timer.stop();
    reset_timer.start();
}

void AudioDialog::largeDecPressed()
{
    config->setCenterFrequency(config->CenterFreq() - 1.0e3 * factor);
    incrementFactor();
    reset_timer.stop();
    reset_timer.start();
}

//
// Callback we only handle this when data is done
//
void CALLBACK waveOutProc(HWAVEOUT hwo,
                          UINT uMsg,
                          DWORD_PTR dwInstance,
                          DWORD_PTR dwParam1,
                          DWORD_PTR dwParam2)
{
    // pointer to free block counter
    DWORD_PTR* freeBlockCounter = (DWORD_PTR*)dwInstance;

    // Only handle certain callbacks
    if(uMsg != WOM_DONE)
        return;

    // We now have one more free block
    EnterCriticalSection(&waveCriticalSection);
    (*freeBlockCounter)++;
    LeaveCriticalSection(&waveCriticalSection);
}

//
// Fill one block
//
void writeAudio(HWAVEOUT hWaveOut, LPSTR data, int size)
{
    WAVEHDR* current;
    current = &waveBlocks[waveCurrentBlock];

    // Unprepare before prepare
    if(current->dwFlags & WHDR_PREPARED)
        waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));

    memcpy(current->lpData, data, size); // Copy data
    waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR)); // Prepare
    waveOutWrite(hWaveOut, current, sizeof(WAVEHDR)); // Write

    EnterCriticalSection(&waveCriticalSection);
    waveFreeBlockCount--;
    LeaveCriticalSection(&waveCriticalSection);

    //while(!waveFreeBlockCount) {
        //cout << "Sleeping\n";
    //    Sleep(10);
    //}

    waveCurrentBlock++;
    waveCurrentBlock %= BLOCK_COUNT;
    current = &waveBlocks[waveCurrentBlock];
}

//
// One time block allocation
//
WAVEHDR* allocateBlocks(int size, int count)
{
    char *buffer;
    int i;
    WAVEHDR* blocks;
    DWORD totalBufferSize = (size + sizeof(WAVEHDR)) * count;

    // allocate memory for the entire set in one go
    buffer = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, totalBufferSize);

    // and set up the pointers to each bit
    blocks = (WAVEHDR*)buffer;
    buffer += sizeof(WAVEHDR) * count;

    for(i = 0; i < count; i++)
    {
        blocks[i].dwBufferLength = size;
        blocks[i].lpData = buffer;
        blocks[i].dwFlags = 0;
        buffer += size;
    }
    return blocks;
}

void freeBlocks(WAVEHDR* blockArray)
{
    HeapFree(GetProcessHeap(), 0, blockArray);
}


