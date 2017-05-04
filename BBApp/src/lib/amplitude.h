#ifndef AMPLITUDE_H
#define AMPLITUDE_H

#include "macros.h"

#include <QString>
#include <QSettings>

#define UNIT_COUNT 4

// Number of enums must match UNIT_COUNT
// Enumes must align with combo_box selection
enum AmpUnits {
    DBM = 0,
    DBMV = 1,
    DBUV = 2,
    MV = 3
};

typedef double (*pfn_convert)(double);
extern char* amp_unit_string[UNIT_COUNT];

inline double DBMtoMW(double dbm) { return pow(10, dbm * 0.1); }
inline double MWtoDBM(double mw) { return 10.0 * log10(mw); }
inline double MVtoMV2(double mv) { return mv * mv; }
inline double MV2toMV(double mv2) { return sqrt(mv2); }

extern pfn_convert cvt_table[UNIT_COUNT][UNIT_COUNT];
double unit_convert(double in, AmpUnits unit_in, AmpUnits unit_out);

/* Amplitude class to represent values
 *  such as dBM, dBuV, mV
 * Not meant to be used for traces, just for
 *  conveniently storing and retrieving string values
 *  for amplitudes
 *
 * Stores the units as well.
 * Reference level will be the reference units type
 */
class Amplitude {
public:
    // Constructors
    Amplitude(double a = 0.0, AmpUnits u = DBM)
        : amplitude(a), units(u) {}
    Amplitude(const Amplitude &other)
        : amplitude(other.Val()), units(other.Units()) {}
    ~Amplitude() {}
    operator double() { return amplitude; }

    void Load(const QSettings &s, const QString &key) {
        *this = Amplitude(s.value(key, Val()).toDouble(),
                          (AmpUnits)s.value(key + "Units", Units()).toInt());
    }

    void Save(QSettings &s, const QString &key) const {
        s.setValue(key, Val());
        s.setValue(key + "Units", Units());
    }

    // Comparators, compare values in similar units
    bool operator<(const Amplitude &other) const {
        Amplitude conv_other = Convert(other);
        return (Val() < Convert(other).Val()) ?
                    true : false;
    }
    bool operator>(const Amplitude &other) const {
        return (Val() > Convert(other).Val()) ?
                    true : false;
    }

    // Assignment, flat copy, no conversion here
    // Use Convert()/ConvertTo() for unit conversions
    Amplitude& operator=(const Amplitude &other) {
        amplitude = other.Val();
        units = other.Units();
        return *this;
    }

    // Equivalence
    bool operator==(const Amplitude &other) const {
        if(amplitude != other.Val()) return false;
        if(units != other.Units()) return false;
        return true;
    }
    bool operator!=(const Amplitude &other) const {
        return !(*this == other);
    }
    Amplitude& operator+=(double val) {
        amplitude += val;
        return *this;
    }
    Amplitude& operator-=(double val) {
        amplitude -= val;
        return *this;
    }
//    Amplitude operator-(const Amplitude &other) {
//        return Amplitude(this->Val() - other.Val());
//    }

    bool IsLogScale() const { return (units != MV); }

    // Getters
    double Val() const { return amplitude; }
    AmpUnits Units() const { return units; }

    // Keep same units, clamp to max val, min and max
    //  may be different units
    void Clamp(Amplitude min, Amplitude max) {
        double min_val = cvt_table[min.Units()][Units()](min.Val());
        double max_val = cvt_table[max.Units()][Units()](max.Val());

        if(amplitude < min_val) amplitude = min_val;
        if(amplitude > max_val) amplitude = max_val;
    }

    // Return a new amplitude using the amplitude of 'this' and
    //  the units of 'other'
    Amplitude Convert(const Amplitude &other) const {
        return Amplitude(
                    cvt_table[Units()][other.Units()](other.Val()),
                other.Units());
    }

    // Used when you want to keep the same units, but acquire
    //  the amplitude of another
    void ConvertTo(const Amplitude &other) {
        amplitude = Convert(other).Val();
    }

    // Get the double portion of *this converted to new_units
    Amplitude ConvertToUnits(AmpUnits new_units) const {
        //return cvt_table[Units()][new_units](Val());
        return Amplitude(cvt_table[Units()][new_units](Val()), new_units);
    }

    // Printing
    QString GetString() const {
        QString s;
        s.sprintf("%.3f ", amplitude);
        return s + amp_unit_string[(int)units];
    }

    // Get string of the value without the supplied unit string
    QString GetValueString() const {
        QString s;
        s.sprintf("%.3f ", amplitude);
        return s;
    }

private:
    double amplitude;
    AmpUnits units;

private:
};


#endif // AMPLITUDE_H
