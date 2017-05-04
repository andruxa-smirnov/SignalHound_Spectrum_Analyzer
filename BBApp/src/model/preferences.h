#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QApplication>
#include <QSettings>
#include <QFile>

#include "lib/bb_lib.h"

#if _WIN64
static const int platformMaxFileSize = 4;
#else
static const int platformMaxFileSize = 1;
#endif

// Keep everything public for now
class Preferences {
public:
    Preferences() { Load(); }
    ~Preferences() { Save(); }

    void SetDefault() {
        programStyle = LIGHT_STYLE_SHEET;

        playbackDelay = 64;
        playbackMaxFileSize = platformMaxFileSize;

        trace_width = 1.0;
        graticule_width = 1.0;
        graticule_stipple = true;

        sweepDelay = 0;
        realTimeFrameRate = 30;
    }

    void Load() {
        QSettings s(QSettings::IniFormat, QSettings::UserScope,
                    "SignalHound", "Preferences");

        int style = s.value("ProgramStyle", BLUE_STYLE_SHEET).toInt();
        SetProgramStyle(style);

        playbackDelay = s.value("PlaybackPrefs/Delay", 64).toInt();
        playbackMaxFileSize = s.value("PlaybackPrefs/MaxFileSize", platformMaxFileSize).toInt();

        trace_width = s.value("ViewPrefs/TraceWidth", 1.0).toFloat();
        graticule_width = s.value("ViewPrefs/GraticuleWidth", 1.0).toFloat();
        graticule_stipple = s.value("ViewPrefs/GraticuleStipple", true).toBool();

        sweepDelay = s.value("SweepPrefs/Delay", 0).toInt();
        realTimeFrameRate = s.value("SweepPrefs/RealTimeFrameRate", 30).toInt();
    }

    void Save() const {
        QSettings s(QSettings::IniFormat, QSettings::UserScope,
                    "SignalHound", "Preferences");

        s.setValue("ProgramStyle", programStyle);

        s.setValue("PlaybackPrefs/Delay", playbackDelay);
        s.setValue("PlaybackPrefs/MaxFileSize", playbackMaxFileSize);

        s.setValue("ViewPrefs/TraceWidth", trace_width);
        s.setValue("ViewPrefs/GraticuleWidth", graticule_width);
        s.setValue("ViewPrefs/GraticuleStipple", graticule_stipple);

        s.setValue("SweepPrefs/Delay", sweepDelay);
        s.setValue("SweepPrefs/RealTimeFrameRate", realTimeFrameRate);
    }

    QString GetDefaultSaveDirectory() const;
    void SetDefaultSaveDirectory(const QString &dir);

    void SetProgramStyle(int newStyle) {
        QFile styleSheet;

        programStyle = newStyle;

        switch(programStyle) {
        case DARK_STYLE_SHEET:
            styleSheet.setFileName(":/style_sheet_dark.css");
            break;
        case LIGHT_STYLE_SHEET:
            styleSheet.setFileName(":/style_sheet_light.css");
            break;
        case BLUE_STYLE_SHEET:
        default:
            styleSheet.setFileName(":/style_sheet_blue.css");
            break;
        }

        if(!styleSheet.open(QFile::ReadOnly)) {
            return;
        }

        QString styleString = styleSheet.readAll();
        styleSheet.close();

        static_cast<QApplication*>
                (QApplication::instance())->
                setStyleSheet(styleString);

    }

    int programStyle;

    int playbackDelay; // In ms [32, 2048]
    int playbackMaxFileSize; // In GB [1, 256]

    // Range for trace_width [1.0, 5.0]
    float trace_width;
    float graticule_width;
    bool graticule_stipple;

    // Arbitrary sweep delay
    int sweepDelay; // In ms [0, 2048]
    int realTimeFrameRate; // In fps [30 - 250]
};

#endif // PREFERENCES_H
