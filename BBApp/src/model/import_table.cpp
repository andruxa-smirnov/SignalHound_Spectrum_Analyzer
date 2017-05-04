#include "import_table.h"

#include <QFile>
#include <fstream>

ImportTable::ImportTable()
{
    active = false;
    stored = false;
}

ImportTable::~ImportTable()
{
    Clear();
}

bool ImportTable::Import(const QString &fileName)
{
    QFile import_file(fileName);
    import_file.open(QIODevice::ReadOnly);

    if(!import_file.isOpen()) {
        return false;
    }

    // The only point at which we clear the old file points
    points.clear();

    // Max line length = 255 + '\0'
    char line[256];
    double v1, v2 = -250.0, v3 = -250.0;
    double lastv2 = v2, lastv3 = v3;

    while(import_file.readLine(line, 256) > 0) {
        int valuesRead = sscanf(line, "%lf, %lf, %lf", &v1, &v2, &v3);
        if(valuesRead == 2) {
            points.push_back(TableEntry(v1 * 1.0e6, lastv2, v2));
        } else if(valuesRead == 3) {
            points.push_back(TableEntry(v1 * 1.0e6, v2, v3));
        } else {
            // Ignore line for now?
        }
        lastv2 = v2;
        lastv3 = v3;
    }

    import_file.close();

    if(points.size() > 0) {
        qSort(points);
        active = true;
        stored = false;
    } else {
        active = false;
    }

    return true;
}

void ImportTable::Clear()
{
    active = false;
}

void ImportTable::BuildStore(const Trace *t)
{
    if(!active) return;

    int i = 0; // Indexing our ctrl points
    int j = 0; // Index our trace
    double start = t->StartFreq();
    double step = t->BinSize();
    double stop = start + (step * t->Length());
    double currFreq = start;

    store.SetSize(t->Length());

    if(points.empty()) {
        return; // No ctrl points
    }

    if(points[0].Freq() > stop) {
        return ; // ctrl points all after stop
    }

    // First control point must be after start freq
    while(points[i].Freq() < start) {
        i++;
        if(i >= points.size()) {
            return; // points all before start
        }
    }

    // Flat up to first point
    while(currFreq < points[i].Freq()) {
        store.Min()[j] = points[i].Min();
        store.Max()[j] = points[i].Max();
        j++;
        currFreq += step;
        if(j >= t->Length()) {
            break;
        }
    }

    // Do point interpolations
    while(currFreq < stop && j < t->Length() ) {
        int k = i + 1; // k == next ctrl point
        if(k >= points.size()) {
            break;
        }

        // Find i and k, such that they straddle currFreq
        while(currFreq >= points[k].Freq()) {
            i++; // Passed a point
            k = i + 1;
            if(currFreq >= stop) {
                break;
            }
            if(k >= points.size()) {
                break;
            }
        }

        if(k >= points.size() || currFreq >= points[k].Freq()) { /*points[k].Freq() >= stop*/
            break;  // We are at the last point, or next point is outside stop
        }

        double p = (currFreq - points[i].Freq()) /
                (points[k].Freq() - points[i].Freq());
        store.Min()[j] = bb_lib::lerp(points[i].Min(), points[k].Min(), p);
        store.Max()[j] = bb_lib::lerp(points[i].Max(), points[k].Max(), p);
        j++;
        currFreq += step;
    }

    // Flat off the back
    while(j < t->Length()) {
        if(i >= points.size()) break;
        store.Min()[j] = points[i].Min();
        store.Max()[j] = points[i].Max();
        j++;
        currFreq += step;
    }

    // Do unit conversions in repective derived class

    stored = true;

    // Debug: print the path loss table
    //std::ofstream f("out.csv");
    //for(int i = 0; i < store.Length(); i++) {
    //    f << i << ", " << store.Max()[i] << "\n";
    //}

    return;
}

void PathLossTable::Apply(Trace *in)
{
    if(!active) return;
    if(!stored || (in->Length() != store.Length())) {
        BuildStore(in);
        if(!in->GetSettings()->RefLevel().IsLogScale()) {
            bb_lib::db_to_lin(store.Max(), store.Length());
        }
    }

    // In the event this check fails, do nothing for now
    if(in->Length() != store.Length()) {
        return;
    }

    if(in->GetSettings()->RefLevel().IsLogScale()) {
        for(int i = 0; i < in->Length(); i++) {
            in->Max()[i] += store.Max()[i];
            in->Min()[i] += store.Max()[i];
        }
    } else {
        for(int i = 0; i < in->Length(); i++) {
            in->Max()[i] *= store.Max()[i];
            in->Min()[i] *= store.Max()[i];
        }
    }
}

void LimitLineTable::Apply(Trace *in)
{
    if(!active) return;
    if(!stored || (in->Length() != store.Length())) {
        BuildStore(in);
        if(!in->GetSettings()->RefLevel().IsLogScale()) {
            bb_lib::dbm_to_mv(store.Max(), store.Length());
        }
    }

    if(in->Length() != store.Length()) {
        return;
    }

    passed = false;

    for(int i = 0; i < in->Length(); i++) {
        if(in->Min()[i] < store.Min()[i] || in->Max()[i] > store.Max()[i]) {
            return;
        }
    }

    passed = true;
}
