#include "savestate.h"

namespace {
	template <typename T>
	static gsl::span<const gsl::byte> asBytes(const T& v)
	{
		return gsl::as_bytes(gsl::span<const T>(&v, 1));
	}

	template <typename T>
	static gsl::span<gsl::byte> asWritableBytes(T& v)
	{
		return gsl::as_writable_bytes(gsl::span<T>(&v, 1));
	}
}

SaveState::SaveState()
{
}

SaveState::SaveState(gsl::span<const gsl::byte> bytes)
{
	SerializerOptions options;
	options.version = 1;
	Deserializer s(bytes, options);

	s >> *this;
}

Bytes SaveState::toBytes() const
{
	SerializerOptions options;
	options.version = 1;
	return Serializer::toBytes(*this, options);
}

void SaveState::serialize(Serializer& s) const
{
	Header header;
	memcpy(header.id.data(), "RGSST", 6);
	header.version = 1;

	// Header
	s << asBytes(header);

	// Timestamp
	s << ChunkId::Timestamp;
	s << Serializer::getSize(timestampChunk);
	s << timestampChunk;

	// ScreenShot
	s << ChunkId::Screenshot;
	s << Serializer::getSize(imageDataChunk);
	s << imageDataChunk;

	// SaveData
	s << ChunkId::SaveData;
	s << Serializer::getSize(saveDataChunk);
	s << saveDataChunk;
}

void SaveState::deserialize(Deserializer& s)
{
	Header header;
	s >> asWritableBytes(header);
	if (memcmp(header.id.data(), "RGSST", 6) != 0) {
		throw Exception("Invalid save state file", 0);
	}
	if (header.version > 1) {
		throw Exception("Don't know how to read save state version, are you up to date?", 0);
	}

	// Read each chunk
	while (s.getBytesLeft() > 0) {
		ChunkId id;
		size_t len;
		s >> id;
		s >> len;

		switch (id) {
		case ChunkId::Timestamp:
			s >> timestampChunk;
			break;

		case ChunkId::SaveData:
			s >> saveDataChunk;
			break;

		case ChunkId::Screenshot:
			s >> imageDataChunk;
			break;

		default:
			s.skipBytes(len);
			break;
		}
	}
}

void SaveState::setSaveData(const Bytes& bytes)
{
	saveDataChunk.origSize = static_cast<uint32_t>(bytes.size());
	saveDataChunk.data = Compression::lz4Compress(bytes.byte_span());
}

Bytes SaveState::getSaveData() const
{
	Bytes result;
	result.resize(saveDataChunk.origSize);
	if (const auto nBytes = Compression::lz4Decompress(saveDataChunk.data.byte_span(), result.byte_span())) {
		result.resize(*nBytes);
		return result;
	} else {
		return {};
	}
}

void SaveState::setScreenShot(const Image& image, float aspectRatio, uint8_t rotation)
{
	imageDataChunk.data = image.savePNGToBytes();
	imageDataChunk.aspectRatio = aspectRatio;
	imageDataChunk.rotation = rotation;
}

std::unique_ptr<Image> SaveState::getScreenShot() const
{
	return std::make_unique<Image>(imageDataChunk.data.byte_span());
}

float SaveState::getScreenShotAspectRatio() const
{
	return imageDataChunk.aspectRatio;
}

uint8_t SaveState::getScreenShotRotation() const
{
	return imageDataChunk.rotation;
}

uint64_t SaveState::getTimeStamp() const
{
	return timestampChunk.timestamp;
}

uint32_t SaveState::getTimePlayed() const
{
	return timestampChunk.timePlayed;
}

void SaveState::setTimeStamp(uint64_t t)
{
	timestampChunk.timestamp = t;
}

void SaveState::setTimePlayed(uint32_t t)
{
	timestampChunk.timePlayed = t;
}

void SaveState::TimestampChunk::serialize(Serializer& s) const
{
	s << timestamp;
	s << timePlayed;
}

void SaveState::TimestampChunk::deserialize(Deserializer& s)
{
	s >> timestamp;
	s >> timePlayed;
}

void SaveState::SaveDataChunk::serialize(Serializer& s) const
{
	s << origSize;
	s << data;
}

void SaveState::SaveDataChunk::deserialize(Deserializer& s)
{
	s >> origSize;
	s >> data;
}

void SaveState::ImageDataChunk::serialize(Serializer& s) const
{
	s << data;
	s << aspectRatio;
	s << rotation;
}

void SaveState::ImageDataChunk::deserialize(Deserializer& s)
{
	s >> data;
	s >> aspectRatio;
	s >> rotation;
}
