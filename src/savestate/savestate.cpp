#include "savestate.h"

SaveState::SaveState()
{
}

SaveState::SaveState(gsl::span<const gsl::byte> bytes)
{
	// TODO
}

void SaveState::setData(Bytes bytes)
{
	// TODO
}

Bytes SaveState::getData() const
{
	// TODO
	return {};
}

void SaveState::setScreenShot(const Image& image)
{
	// TODO
}

std::unique_ptr<Image> SaveState::getScreenShot() const
{
	// TODO
	return {};
}

Bytes SaveState::toBytes() const
{
	// TODO
	return {};
}
