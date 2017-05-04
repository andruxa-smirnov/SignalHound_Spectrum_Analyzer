#include "amplitude.h"
#include <cmath>

// Index of unit must match enum value
char* amp_unit_string[UNIT_COUNT] = { "dBm", "dBmV", "dBuV", "mV" };

double no_change(double in) { return in; }

double DBMtoDBMV(double dbm) { return dbm + 46.9897; }
double DBMtoDBUV(double dbm) { return dbm + 106.9897; }
double DBMtoMV(double dbm) { return pow(10.0,((dbm + 46.9897) / 20.0)); }

double DBMVtoDBM(double dbmv) { return dbmv - 46.9897; }
double DBMVtoDBUV(double dbmv){ return dbmv + 60; }
double DBMVtoMV(double dbmv) { return pow(10, dbmv * 0.05); }

double DBUVtoDBM(double dbuv) { return dbuv - 106.9897; }
double DBUVtoDBMV(double dbuv) { return dbuv - 60; }
double DBUVtoMV(double dbuv) { return pow(10.0,((dbuv - 60.0) / 20.0)); }

double MVtoDBM(double mv) { return 20 * log10(mv) - 46.9897; }
double MVtoDBMV(double mv) { return 20 * log10(mv); }
double MVtoDBUV(double mv) { return 20 * log10(mv) + 60.0; }

pfn_convert cvt_table[UNIT_COUNT][UNIT_COUNT] = {
    { no_change, DBMtoDBMV, DBMtoDBUV, DBMtoMV },
    { DBMVtoDBM, no_change, DBMVtoDBUV, DBMVtoMV },
    { DBUVtoDBM, DBUVtoDBMV, no_change, DBUVtoMV },
    { MVtoDBM, MVtoDBMV, MVtoDBUV, no_change }
};

double unit_convert(double in, AmpUnits unit_in, AmpUnits unit_out)
{
    return cvt_table[int(unit_in)][int(unit_out)](in);
}
