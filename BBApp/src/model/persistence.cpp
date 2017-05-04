#include "persistence.h"
#include "trace.h"

#include <QPoint>

const int MAX_PERSIST_W = 512;
const int PERSIST_H = 512;

Persistence::Persistence()
{
    map = nullptr;
    image = nullptr;
    img_width = 0;
}

Persistence::~Persistence()
{
    if(map) {
        delete [] map;
        map = nullptr;
    }
    if(image) {
        delete [] image;
        image = nullptr;
    }
}

float* Persistence::GetImage()
{
    float *trans = new float[img_width * PERSIST_H];

    map_mutex.lock();

    for(int i = 0; i < img_width; i++) {
        for(int j = 0; j < PERSIST_H; j++) {
                trans[j*PERSIST_H + i] = map[i*PERSIST_H + j];
        }
    }

    map_mutex.unlock();

    for(int i = 0; i < img_width * PERSIST_H; i++) {
//        image[i*4] = 1.0; //trans[i];
//        image[i*4+1] = 0.0; //trans[i];
//        if(i & 1) image[i*4+1] = 1.0;
//        image[i*4+2] = 0.0; //trans[i];

        float t = trans[i];

        image[i*4] = trans[i];
        image[i*4+1] = trans[i];
        image[i*4+2] = trans[i];

        if(trans[i] > 0.01) {
            image[i*4+3] = 1.0;
        } else {
            image[i*4+3] = 0.0;
        }
    }


    delete [] trans;

    return image;
}

void Persistence::Reconfigure(const Trace *trace)
{
    int new_len = bb_lib::min2(trace->Length(), MAX_PERSIST_W);

    map_mutex.lock();

    if(img_width != new_len) {
        img_width =  new_len;

        if(map) {
            delete [] map;
            map = nullptr;
        }
        map = new float[img_width * PERSIST_H];

        if(image) {
            delete [] image;
            image = nullptr;
        }
        image = new float[img_width * PERSIST_H * 4];
    }

    map_mutex.unlock();

    Clear();
}

void Persistence::Clear()
{
    for(int i = 0; i < img_width * 512; i++) {
        map[i] = 0.0;
    }
}

/*
 * Turn trace into 512 by 512 min/max list
 *  either by interpolation or bin combination
 * Decay model ->
 */
void Persistence::Accumulate(const Trace *trace)
{    
    if(bb_lib::min2(trace->Length(), MAX_PERSIST_W) != img_width) {
        Reconfigure(trace);
    }

    GLVector trace_vector;
    normalize_trace(trace, trace_vector, QPoint(img_width, 0));

    //for(int i = 0; i < trace->Length(); i++) {
    for(int i = 0; i < img_width; i++) {
        max_ix[i] = trace_vector[i*4+1] * 512;
        min_ix[i] = trace_vector[i*4+3] * 512;
        //min_ix[i] = (short)((trace->Min()[i] + 130.0) * 5.12);
        //max_ix[i] = (short)((trace->Max()[i] + 130.0) * 5.12);

        min_ix[i] = bb_lib::max2(min_ix[i], (short)0);
        min_ix[i] = bb_lib::min2(min_ix[i], (short)511);

        max_ix[i] = bb_lib::max2(max_ix[i], (short)0);
        max_ix[i] = bb_lib::min2(max_ix[i], (short)511);

    }

    // Modify the persistence map
    map_mutex.lock();

    for(int i = 0; i < img_width - 1; i++) {
        float *map_ptr = map + (i * 512);
        // Main min->max line
        for(int j = min_ix[i]; j <= max_ix[i]; j++) {
            map_ptr[j] += 0.04;
        }
        // Draw lines between
        for(int j = max_ix[i]; j < min_ix[i+1]; j++) {
            map_ptr[j] += 0.01;
        }
        for(int j = max_ix[i]; j > max_ix[i+1]; j--) {
            map_ptr[j] += 0.01;
        }
    }

    Decay();

    map_mutex.unlock();
}

// Only call from Accumulate, do not lock
void Persistence::Decay()
{
    for(int i = 0; i < img_width * 512; i++) {
        map[i] *= 0.975;
    }
}
