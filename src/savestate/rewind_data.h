#pragma once

#include <halley.hpp>
using namespace Halley;

class RewindData {
public:
	explicit RewindData(size_t bytes);
    void setCapacity(size_t bytes);

    void pushFrame(Bytes saveState);
    std::optional<Bytes> popFrame();

    Bytes getBuffer(size_t size);

private:
    std::deque<Bytes> frames;
    Vector<Bytes> spareBuffers;
    size_t capacity = 0;
    size_t curSize = 0;

    void compress(Bytes& oldFrame, const Bytes& newFrame);
    void decompress(Bytes& oldFrame, const Bytes& newFrame);
    void dropOldest();
};
