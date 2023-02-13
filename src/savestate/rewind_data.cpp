#include "rewind_data.h"

RewindData::RewindData(size_t bytes)
	: capacity(bytes)
{
}

void RewindData::setCapacity(size_t bytes)
{
	capacity = bytes;
}

void RewindData::pushFrame(Bytes saveState)
{
	frames.push_back(std::move(saveState));
	if (frames.size() >= 2) {
		compress(frames[frames.size() - 2], frames.back());
	}

	while (frames.size() > 2 && curSize > capacity) {
		dropOldest();
	}
	if (frames.size() >= 2) {
		//Logger::logDev("Rewind buffer = " + toString(frames.size()) + " frames [+" + toString(frames[frames.size() - 2].size()) + " bytes]");
	}
}

std::optional<Bytes> RewindData::popFrame()
{
	if (frames.empty()) {
		return {};
	}
	if (frames.size() >= 2) {
		decompress(frames[frames.size() - 2], frames.back());
	}
	auto bytes = std::move(frames.back());
	frames.pop_back();
	return bytes;
}

void RewindData::compress(Bytes& oldFrame, const Bytes& newFrame)
{
	// Delta compress
	assert(oldFrame.size() == newFrame.size());
	size_t n = oldFrame.size();
	for (size_t i = 0; i < n; ++i) {
		oldFrame[i] -= newFrame[i];
	}

	// Deflate
	oldFrame = Compression::compress(oldFrame);

	curSize += oldFrame.size();
}

void RewindData::decompress(Bytes& oldFrame, const Bytes& newFrame)
{
	curSize -= oldFrame.size();

	// Inflate
	oldFrame = Compression::decompress(oldFrame);

	// Delta decompress
	assert(oldFrame.size() == newFrame.size());
	size_t n = oldFrame.size();
	for (size_t i = 0; i < n; ++i) {
		oldFrame[i] += newFrame[i];
	}
}

void RewindData::dropOldest()
{
	curSize -= frames.front().size();
	frames.pop_front();
}
