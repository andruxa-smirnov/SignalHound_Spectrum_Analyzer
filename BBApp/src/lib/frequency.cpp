#include "frequency.h"

struct StringToHertz {
    QString string;
    Frequency::Hertz hertz;
};

// lowercase only, never deal with upper
StringToHertz str_to_hz[] = {
    {"h", Frequency::Hz},
    {"hz", Frequency::Hz},
    {"k", Frequency::kHz},
    {"kh", Frequency::kHz},
    {"khz", Frequency::kHz},
    {"m", Frequency::MHz},
    {"mh", Frequency::MHz},
    {"mhz", Frequency::MHz},
    {"g", Frequency::GHz},
    {"gh", Frequency::GHz},
    {"ghz", Frequency::GHz}
};

// Splits an input string into a number and unit_str
// If an invalid input is detected, return false
// Strings returned are lowercase
static bool split_input_string(QString str,
                               QString &number,
                               QString &unit_str)
{
    // be safe ? or overwrite ?
    number.clear();
    unit_str.clear();

    // Remove all whitespace
    str.remove(' ');

    // Input cannot be empty or start with letter
    if(str.length() == 0)
        return false;
    if(!str[0].isDigit())
        return false;

    // Get numeric value
    int ix = 0;
    for(; ix < str.length(); ix++) {
        if(!str[ix].isDigit() && !(str[ix] == '.'))
            break;
    }
    number = str.left(ix);

    if(ix == str.length()) {
        unit_str = "hz";
    } else {
        unit_str = str.right(str.length() - ix);
    }
    unit_str = unit_str.toLower();

    return true;
}

// Using strings returned from bb_lib::split_input_str
// If unit string is invalid return false
// else return true, 'f' contains freq
// Input strings must be lowercase
static bool get_freq_from_str(const QString &number,
                              const QString &unit_str,
                              Frequency &f)
{
    bool ok;
    double val = number.toDouble(&ok);
    if(!ok)
        return false;

    for(int i = 0; i < sizeof(str_to_hz) / sizeof(StringToHertz); i++) {
        if(str_to_hz[i].string == unit_str) {
            f.Set(val, str_to_hz[i].hertz);
            return true;
        }
    }

    return false;
}

// static function determine if 'str' represents a valid frequency
// return true if str is valid freq string
bool Frequency::IsValidFreqString(QString str, Frequency &f)
{
    QString number, unit_str;

    // Remove all whitespace
    str.remove(' ');
    if(str.length() <= 0)
        return false;

    // Get numeric value
    int ix = 0;
    for(; ix < str.length(); ix++) {
        if(!str[ix].isDigit() && !(str[ix] == '.'))
            break;
    }
    number = str.left(ix);

    // Get unit str, or use prev unit string
    if(ix == str.length()) {
        unit_str = f.GetUnitString();
    } else {
        unit_str = str.right(str.length() - ix);
    }
    unit_str = unit_str.toLower();

    // Is it a number ?
    bool ok;
    double val = number.toDouble(&ok);
    if(!ok)
        return false;

    // Create new frequency
    for(int i = 0; i < sizeof(str_to_hz) / sizeof(StringToHertz); i++) {
        if(str_to_hz[i].string == unit_str) {
            f.Set(val, str_to_hz[i].hertz);
            break;
        }
    }

    return true;

//    if(!split_input_string(str, number, unit_str))
//        return false;

//    return get_freq_from_str(number, unit_str, f);
}

void Frequency::Set(double f, Hertz h)
{
    switch(h) {
    case Hz: frequency = f; break;
    case kHz: frequency = f * 1.0e3; break;
    case MHz: frequency = f * 1.0e6; break;
    case GHz: frequency = f * 1.0e9; break;
    }
}

Frequency::Hertz Frequency::GetUnits() const
{
    double abs_freq = fabs(frequency);
    if(abs_freq < 1.0e3) return Hz;
    if(abs_freq < 1.0e6) return kHz;
    if(abs_freq < 1.0e9) return MHz;
    return GHz;
}

QString Frequency::GetUnitString() const
{
    switch(GetUnits()) {
    case Hz: return "Hz";
    case kHz: return "kHz";
    case MHz: return "MHz";
    case GHz: return "GHz";
    }

    return "Hz";
}
