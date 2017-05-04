#ifndef THREADSAFE_QUEUE_H
#define THREADSAFE_QUEUE_H

#include "bb_lib.h"

#include <mutex>
#include <array>
#include <atomic>

// "Queue" is implemented as a array with rolling indices
// This is to reduce memory (de)allocation
// Only used for storing a circular buffer of vectors for
//   a real-time trace buffer

// Manually synchronized, push new things on the front
//   before incrementing the size.
// Pull things off the back, incrementing each time, until
//   a nullptr is returned.

template<class _Type, int _Size>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() {
        front = 0;
        back = 0;
    }

    ~ThreadSafeQueue() {}

    _Type* Front() {
        return &safe_queue[front];
    }

    // Front allowed to wrap on back
    // This would be a loss of data
    void IncrementFront() {
        if(front >= (_Size-1)) {
            front = 0;
        } else {
            front += 1;
        }
    }

    _Type* Back() {
        if(back == front)
            return nullptr;
        return &safe_queue[back];
    }

    void IncrementBack() {
        if(back == front)
            return;
        if(back >= (_Size-1)) {
            back = 0;
        } else {
            back += 1;
        }
    }

    int Size() const { return _Size; }

private:
    std::atomic<int> front, back;
    std::array<_Type, _Size> safe_queue;
};

#endif // THREADSAFE_QUEUE_H
