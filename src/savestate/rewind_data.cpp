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

Bytes RewindData::getBuffer(size_t size)
{
	Bytes buffer;
	if (!spareBuffers.empty()) {
		buffer = std::move(spareBuffers.back());
		spareBuffers.pop_back();
	}
	buffer.resize(size);
	return buffer;
}

void RewindData::compress(Bytes& oldFrame, const Bytes& newFrame)
{
	// Delta compress
	assert(oldFrame.size() == newFrame.size());
	size_t n = oldFrame.size();
	auto* oldData = oldFrame.data();
	const auto* newData = newFrame.data();
	for (size_t i = 0; i < n; ++i) {
		oldData[i] -= newData[i];
	}

	// Deflate
	compressBuffer.resize(std::max(compressBuffer.size(), oldFrame.size() + 16));
	const auto size = Compression::lz4Compress(oldFrame, compressBuffer);
	spareBuffers.push_back(std::move(oldFrame));
	oldFrame = Bytes();
	oldFrame.resize(size);
	memcpy(oldFrame.data(), compressBuffer.data(), size);

	curSize += size;
}

void RewindData::decompress(Bytes& oldFrame, const Bytes& newFrame)
{
	curSize -= oldFrame.size();

	// Inflate
	auto size = Compression::lz4Decompress(oldFrame, compressBuffer);
	oldFrame = getBuffer(*size);
	memcpy(oldFrame.data(), compressBuffer.data(), *size);

	// Delta decompress
	assert(oldFrame.size() == newFrame.size());
	size_t n = oldFrame.size();
	auto* oldData = oldFrame.data();
	const auto* newData = newFrame.data();
	for (size_t i = 0; i < n; ++i) {
		oldData[i] += newData[i];
	}

}

void RewindData::dropOldest()
{
	curSize -= frames.front().size();
	frames.pop_front();
}
