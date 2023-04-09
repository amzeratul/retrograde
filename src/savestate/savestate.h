#pragma once

#include <halley.hpp>
using namespace Halley;

class SaveState {
public:
	SaveState();
	SaveState(gsl::span<const gsl::byte> bytes);

	void setData(Bytes bytes);
	Bytes getData() const;

	void setScreenShot(const Image& image);
	std::unique_ptr<Image> getScreenShot() const;

	Bytes toBytes() const;

private:
	Bytes saveData;
	Bytes imageData;
};
