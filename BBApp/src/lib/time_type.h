#ifndef TIME_TYPE_H
#define TIME_TYPE_H

#include <QString>
#include <cmath>

// For equivalence, 1us error
const double time_epsilon = 1.0e-6;

enum TimeUnit {
    MICROSECOND = 0,
    MILLISECOND,
    SECOND
};

struct TimeUnitMap {
    TimeUnit unit;
    char *unit_str;
};

extern TimeUnitMap g_time_unit_map[];

/*
 * Represents an arbitrary time
 * Used instead of a bunch of random doubles
 * String printing as well
 *
 * Base unit is seconds, everything is converted from there
 */
class Time {
public:
    // Constructor
    explicit Time(double t = 0.0) : seconds(t) {}
    Time(const Time &other) { *this = other; }
    Time(double t, TimeUnit tu);

    // Conversions
    Time& operator=(double t) { seconds = t; return *this; }
    Time& operator=(const Time &other) { seconds = other.Val(); return *this; }
    operator double() { return seconds; }

    // Comparators
    bool operator==(const Time &other) const {
        return fabs(seconds - other.Val()) < time_epsilon;
    }
    bool operator!=(const Time &other) const {
        return !(*this == other);
    }
    bool operator<(const Time &other) const {
        return seconds < other.Val();
    }
    bool operator>(const Time &other) const {
        return seconds > other.Val();
    }

    double Val() const { return seconds; }
    // Return the double in a different unit type
    double ChangeUnit(TimeUnit tu) const;

    void Clamp(double min_seconds, double max_seconds) {
        if(seconds < min_seconds) seconds = min_seconds;
        if(seconds > max_seconds) seconds = max_seconds;
    }

    // Get string with units, ms or s only
    QString GetString() const {
        QString s;
        if(seconds < 0.0) s.sprintf("%.3f ms", seconds * 0.001);
        else s.sprintf("%f s", seconds);

        return s;
    }

private:
    double seconds;
};

#endif // TIME_TYPE_H
