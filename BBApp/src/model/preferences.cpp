#include "preferences.h"
#include "lib/bb_lib.h"

QString Preferences::GetDefaultSaveDirectory() const
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "SignalHound", "Preferences");

    return s.value("DefaultSaveLocation", bb_lib::get_my_documents_path()).toString();
}

void Preferences::SetDefaultSaveDirectory(const QString &dir)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "SignalHound", "Preferences");

    s.setValue("DefaultSaveLocation", dir);
}
