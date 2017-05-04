#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "../lib/macros.h"

#include <vector>
#include <array>
#include <mutex>

class Trace;

class Persistence {
public:
    Persistence();
    ~Persistence();

    float* GetMap() const { return map; }
    float* GetImage();

    int Width() const { return img_width; }
    int Height() const { return 512; }

    // Reconfigure the persistence class to prepare
    //  for a new trace size/settings
    void Reconfigure(const Trace *trace);

    // Reset the map
    void Clear();

    // Contribute one trace to the map
    void Accumulate(const Trace *trace);

protected:

private:
    void Decay();

    int img_width;
    // Temporarily stores traces converted to indices
    //std::vector<short> min_ix, max_ix;
    std::array<short, 512> min_ix, max_ix;
    // Single channel persistence map, layout out horozontally
    float *map;
    // Transposed image color map, only populated when requested to
    //   be drawn
    float *image;

    // Lock out when modifying the map
    std::mutex map_mutex;

private:
    DISALLOW_COPY_AND_ASSIGN(Persistence)
};

#endif // PERSISTENCE_H
