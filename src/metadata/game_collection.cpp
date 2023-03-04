#include "game_collection.h"

#include <filesystem>

GameCollection::GameCollection(Path dir)
	: dir(std::move(dir))
{
}

void GameCollection::scanGames()
{
	entries.clear();

	std::error_code ec;
	for (const auto& e: std::filesystem::directory_iterator(dir.getNativeString().cppStr(), ec)) {
		if (e.is_regular_file()) {
			auto path = e.path().filename().string();
			entries.push_back(makeEntry(path));
		}
	}
}

gsl::span<const GameCollection::Entry> GameCollection::getEntries() const
{
	return entries;
}

GameCollection::Entry GameCollection::makeEntry(const Path& path) const
{
	Entry result;
	result.filename = path;
	result.displayName = path.replaceExtension("").getFilename().getString();
	return result;
}
