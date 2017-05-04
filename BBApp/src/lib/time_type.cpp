#include "time_type.h"

TimeUnitMap g_time_unit_map[] = {
    { MICROSECOND, "us" },
    { MILLISECOND, "ms" },
    { SECOND     , "s" }
};

// Construct from time in a different unit type
Time::Time(double t, TimeUnit tu)
{
    switch(tu) {
    case SECOND:
        seconds = t;
        break;
    case MILLISECOND:
        seconds = t / 1.0e3;
        break;
    case MICROSECOND:
        seconds = t / 1.0e6;
        break;
    }
}

// Return the double portion of the Time in a different unit type
double Time::ChangeUnit(TimeUnit tu) const
{
    switch(tu) {
    case SECOND: return seconds;
    case MILLISECOND: return seconds * 1.0e3;
    case MICROSECOND: return seconds * 1.0e6;
    }
    return seconds;
}

