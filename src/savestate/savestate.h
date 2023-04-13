#pragma once

#include <halley.hpp>
using namespace Halley;

class SaveState {
public:
	SaveState();
	SaveState(gsl::span<const gsl::byte> bytes);

	void setSaveData(const Bytes& bytes);
	Bytes getSaveData() const;

	void setScreenShot(const Image& image, float aspectRatio, uint8_t rotation);
	std::unique_ptr<Image> getScreenShot() const;
	float getScreenShotAspectRatio() const;
	uint8_t getScreenShotRotation() const;

	uint64_t getTimeStamp() const;
	uint32_t getTimePlayed() const;
	void setTimeStamp(uint64_t t);
	void setTimePlayed(uint32_t t);

	Bytes toBytes() const;

	void serialize(Serializer& s) const;
	void deserialize(Deserializer& s);

private:
	struct Header {
		std::array<char, 6> id;
		uint16_t version;
	};

	enum class ChunkId : uint8_t {
		Timestamp,
		Screenshot,
		SaveData
	};

	struct TimestampChunk {
		uint64_t timestamp = 0;
		uint32_t timePlayed = 0;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct SaveDataChunk {
		uint32_t origSize = 0;
		Bytes data;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ImageDataChunk {
		Bytes data;
		float aspectRatio;
		uint8_t rotation;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	TimestampChunk timestampChunk;
	SaveDataChunk saveDataChunk;
	ImageDataChunk imageDataChunk;
};
