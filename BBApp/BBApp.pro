#-------------------------------------------------
#
# Project created by QtCreator 2013-11-12T16:55:51
#
#-------------------------------------------------

QT += core gui opengl printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BBApp
TEMPLATE = app

SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/lib/bb_lib.cpp \
    src/lib/amplitude.cpp \
    src/lib/frequency.cpp \
    src/widgets/entry_widgets.cpp \
    src/widgets/dock_panel.cpp \
    src/widgets/dock_page.cpp \
    src/widgets/sweep_panel.cpp \
    src/model/sweep_settings.cpp \
    src/model/session.cpp \
    src/views/sweep_central.cpp \
    src/views/trace_view.cpp \
    src/model/trace.cpp \
    src/model/marker.cpp \
    src/model/device_bb60a.cpp \
    src/model/trace_manager.cpp \
    src/widgets/measure_panel.cpp \
    src/model/persistence.cpp \
    src/model/audio_settings.cpp \
    src/lib/time_type.cpp \
    src/model/playback_toolbar.cpp \
    src/widgets/audio_dialog.cpp \
    src/widgets/status_bar.cpp \
    src/model/device.cpp \
    src/model/import_table.cpp \
    src/widgets/preferences_dialog.cpp \
    src/model/demod_settings.cpp \
    src/views/demod_central.cpp \
    src/views/demod_iq_time_plot.cpp \
    src/widgets/demod_panel.cpp \
    src/kiss_fft/kiss_fft.c \
    src/views/demod_spectrum_plot.cpp \
    src/views/demod_sweep_plot.cpp \
    src/widgets/measuring_receiver_dialog.cpp \
    src/model/device_sa.cpp \
    src/lib/device_traits.cpp \
    src/views/harmonics_central.cpp \
    src/views/gl_sub_view.cpp \
    src/views/harmonics_spectrum.cpp \
    src/views/tg_central.cpp \
    src/views/tg_trace_view.cpp \
    src/widgets/tg_panel.cpp \
    src/views/phase_noise_central.cpp \
    src/views/phase_noise_plot.cpp \
    src/widgets/if_output_dialog.cpp \
    src/widgets/self_test_dialog.cpp \
    src/model/preferences.cpp

HEADERS += src/mainwindow.h \
    src/lib/frequency.h \
    src/lib/macros.h \
    src/lib/time_type.h \
    src/lib/bb_lib.h \
    src/lib/amplitude.h \
    src/widgets/entry_widgets.h \
    src/widgets/dock_panel.h \
    src/widgets/dock_page.h \
    src/widgets/sweep_panel.h \
    src/model/sweep_settings.h \
    src/model/session.h \
    src/views/sweep_central.h \
    src/views/trace_view.h \
    src/model/trace.h \
    src/model/marker.h \
    src/model/device.h \
    src/model/sa_api.h \
    src/model/device_bb60a.h \
    src/lib/bb_api.h \
    src/model/trace_manager.h \
    src/widgets/measure_panel.h \
    src/model/persistence.h \
    src/model/audio_settings.h \
    src/views/idle_view.h \
    src/views/persistence_view.h \
    src/model/color_prefs.h \
    src/model/playback_toolbar.h \
    src/lib/threadsafe_queue.h \
    src/model/preferences.h \
    src/widgets/audio_dialog.h \
    src/widgets/status_bar.h \
    src/widgets/progress_dialog.h \
    src/model/import_table.h \
    src/widgets/preferences_dialog.h \
    src/model/demod_settings.h \
    src/widgets/demod_panel.h \
    src/views/demod_central.h \
    src/views/central_stack.h \
    src/views/demod_iq_time_plot.h \
    src/views/gl_sub_view.h \
    src/kiss_fft/kiss_fft.h \
    src/kiss_fft/kissfft.hh \
    src/views/demod_spectrum_plot.h \
    src/views/demod_sweep_plot.h \
    src/widgets/measuring_receiver_dialog.h \
    src/model/device_sa.h \
    src/lib/sa_api.h \
    src/lib/device_traits.h \
    src/views/harmonics_central.h \
    src/views/harmonics_spectrum.h \
    src/views/tg_central.h \
    src/views/tg_trace_view.h \
    src/widgets/tg_panel.h \
    src/views/phase_noise_central.h \
    src/views/phase_noise_plot.h \
    src/widgets/if_output_dialog.h \
    src/widgets/self_test_dialog.h \
    src/version.h

OTHER_FILES += \
    style_sheet.css \
    todo.txt \
    bb_app.rc \
    spike.ico

LIBS += \
    -Ldebug -lbb_api \
    -Ldebug -lsa_api

INCLUDEPATH += src external_libraries

RC_FILE = bb_app.rc

RESOURCES = ./resources/bb_app.qrc

FORMS +=
