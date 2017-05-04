#ifndef IMPORT_TABLE_H
#define IMPORT_TABLE_H

#include <QVector>
#include "trace.h"

class TableEntry {
public:
    TableEntry() { entry[0] = entry[1] = entry[2] = 0.0; }
    explicit TableEntry(double freq, double min, double max) {
        entry[0] = freq; entry[1] = min; entry[2] = max;
    }
    ~TableEntry() {}

    TableEntry& operator=(const TableEntry &other) {
        entry[0] = other.entry[0];
        entry[1] = other.entry[1];
        entry[2] = other.entry[2];
        return *this;
    }

    // Comparison for qSort()
    friend bool operator<(const TableEntry &left,
                          const TableEntry &right) {
        return left.entry[0] < right.entry[0];
    }

    double Freq() const { return entry[0]; }
    double Min() const { return entry[1]; }
    double Max() const { return entry[2]; }

    double entry[3];
};

/*
 * Base class for the path-loss arrays and limit lines
 */
class ImportTable {
public:
    ImportTable();
    ~ImportTable();

    bool Import(const QString &fileName);
    void Clear();
    // Build a new store using the dimensions of t
    void BuildStore(const Trace *t);
    virtual void Apply(Trace *in) = 0;

    bool Active() const { return active; }

    // Stores the path loss trace and limit lines
    // Path-Loss correction stored in Max portion
    // Limit lines stored in both
    Trace store;

protected:
    // True when a file is loaded
    bool active;
    // True when a store has been built from a trace
    bool stored;

private:
    QVector<TableEntry> points;
};

class PathLossTable : public ImportTable {
public:
    PathLossTable() {}
    ~PathLossTable() {}

    // Add/Multiply the path loss trace to
    void Apply(Trace *in);

private:
};

class LimitLineTable : public ImportTable {
public:
    LimitLineTable() {}
    ~LimitLineTable() {}

    bool LimitsPassed() const { return passed; }

    // Test the input trace against the limits
    void Apply(Trace *in);

private:
    bool passed;
};

#endif // IMPORT_TABLE_H
