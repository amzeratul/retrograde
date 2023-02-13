#pragma once

#include <halley.hpp>
using namespace Halley;

class RewindData {
public:
	explicit RewindData(size_t bytes);
    void setCapacity(size_t bytes);

    void pushFrame(Bytes saveState);
    std::optional<Bytes> popFrame();

private:
    std::deque<Bytes> frames;
    size_t capacity = 0;
    size_t curSize = 0;

    void compress(Bytes& oldFrame, const Bytes& newFrame);
    void decompress(Bytes& oldFrame, const Bytes& newFrame);
    void dropOldest();
};
