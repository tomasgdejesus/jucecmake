#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <atomic>

template<typename T, size_t sz>
class CircularBuffer {
public:
    CircularBuffer() = default;

    // must be thread safe so prevent copy
    CircularBuffer(const CircularBuffer&) = delete;
    CircularBuffer& operator=(const CircularBuffer&) = delete;

     // must only be called by writer
    void add(const T& value) {
        if (clearFlag.exchange(false)) {
            fifo.reset();
            buffer = {};
        }

/// TODO: use write() and read() instead of fifo.prepareTo_ and fifo.finished_, https://docs.juce.com/master/classAbstractFifo.html#a41c705711ad487e127df59cd978ef24c

        int start1, size1, start2, size2;
        fifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 <= 0 && size2 <= 0) { // buffer is full
            fifo.finishedRead(1); // flush oldest
            fifo.prepareToWrite(1, start1, size1, start2, size2);
        }

        if (size1 > 0)
            buffer[start1] = value;
        else if (size2 > 0)
            buffer[start2] = value;

        fifo.finishedWrite(1);
    }

    // must only be called by reader
    std::vector<T> read() {
        std::vector<T> result;

        if (clearFlag.load())
            return result; // clear flag enabled, writer has not updated buffer (yet)

        int start1, size1, start2, size2;
        fifo.prepareToRead(sz, start1, size1, start2, size2);

        for (int i = 0; i < size1; i++)
            result.push_back(buffer[start1 + i]);

        for (int i = 0; i < size2; i++)
            result.push_back(buffer[start2 + i]);

        return result;
    }

    std::atomic<bool> clearFlag{false};

private:
    juce::AbstractFifo fifo{sz};
    std::array<T, sz> buffer{};
};
