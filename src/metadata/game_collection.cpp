#include "game_collection.h"

#include <filesystem>

#include "src/config/core_config.h"

void GameCollection::Entry::sortFiles()
{
	const auto getPriority = [](const String& ext) -> int
	{
		if (ext == ".cue") {
			return 1;
		}
		if (ext == ".zip") {
			return -1;
		}
		if (ext == ".jpg" || ext == ".png" || ext == ".mp4" || ext == ".mp3") {
			return -2;
		}
		return 0;
	};

	std::sort(files.begin(), files.end(), [&] (const Path& a, const Path& b)
	{
		return getPriority(a.getExtension()) > getPriority(b.getExtension());
	});
}

const Path& GameCollection::Entry::getBestFileToLoad(const CoreConfig& coreConfig) const
{
	const auto& blocked = coreConfig.getBlockedExtensions();
	for (const auto& file: files) {
		if (!std_ex::contains(blocked, file.getExtension().mid(1))) {
			return file;
		}
	}
	return files.front();
}

const Path& GameCollection::Entry::getMedia(MediaType type) const
{
	const auto iter = media.find(type);
	if (iter == media.end()) {
		static Path empty;
		return empty;
	}
	return iter->second;
}

GameCollection::GameCollection(Path dir)
	: dir(std::move(dir))
{
}

void GameCollection::scanGames()
{
	entries.clear();
	index.clear();

	std::error_code ec;
	for (const auto& e: std::filesystem::directory_iterator(dir.getNativeString().cppStr(), ec)) {
		if (e.is_regular_file()) {
			makeEntry(e.path().filename().string());
		}
	}
}

gsl::span<const GameCollection::Entry> GameCollection::getEntries() const
{
	return entries;
}

void GameCollection::makeEntry(const Path& path)
{
	auto [displayName, tags] = parseName(path.replaceExtension("").getFilename().getString());

	const auto iter = index.find(displayName);
	if (iter != index.end()) {
		// Game already listed, merge
		auto& entry = entries[iter->second];
		entry.files.push_back(path);
		entry.sortFiles();
	} else {
		Entry result;
		result.files.push_back(path);
		result.displayName = displayName;
		result.tags = std::move(tags);
		index[result.displayName] = entries.size();
		collectMediaData(result);
		entries.push_back(std::move(result));
	}
}

std::pair<String, Vector<String>> GameCollection::parseName(const String& name) const
{
	Vector<char> displayName;
	Vector<String> tags;
	size_t tagStart = 0;
	bool inTag = false;

	// Parse name
	for (size_t i = 0; i < name.length(); ++i) {
		const auto chr = name[i];
		if (chr == '(' || chr == '[') {
			tagStart = i + 1;
			inTag = true;
			continue;
		}
		if (chr == ')' || chr == ']') {
			if (inTag) {
				tags.push_back(String(std::string_view(name).substr(tagStart, i - tagStart)));
				inTag = false;
			}
			continue;
		}
		if (!inTag) {
			if (chr != ' ' || (!displayName.empty() && displayName.back() != ' ')) {
				displayName.push_back(chr);
			}
		}
	}
	String displayNameStr = String(displayName.data(), displayName.size());
	displayNameStr.trimBoth();

	return { displayNameStr, tags };
}

void GameCollection::collectMediaData(Entry& entry)
{
	if (entry.files.empty()) {
		return;
	}

	const auto gameName = entry.files[0].getFilename().replaceExtension("").toString();
	const auto gameDir = dir / entry.files[0].parentPath();
	const auto imageDir = gameDir / "images";

	auto tryAdd = [&](MediaType type, const Path& path)
	{
		if (entry.media.find(type) == entry.media.end() && Path::exists(path)) {
			entry.media[type] = path;
		}
	};

	tryAdd(MediaType::Screenshot, imageDir / (gameName + "-image.png"));
}
