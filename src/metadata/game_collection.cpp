#include "game_collection.h"

#include <filesystem>

#include "es_gamelist.h"
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

bool GameCollection::Entry::operator<(const Entry& other) const
{
	return sortName < other.sortName;
}

GameCollection::GameCollection(Path dir)
	: dir(std::move(dir))
{
}

void GameCollection::scanGames()
{
	entries.clear();
	fileIndex.clear();
	nameIndex.clear();

	std::error_code ec;
	for (const auto& e: std::filesystem::directory_iterator(dir.getNativeString().cppStr(), ec)) {
		if (e.is_regular_file()) {
			makeEntry(e.path().filename().string());
		}
	}
}

void GameCollection::scanGameData()
{
	if (!gameDataRequested) {
		gameDataRequested = true;
		scanningFuture = Concurrent::execute(Executors::getCPU(), [=]()
		{
			doScanGames();
			return true;
		});
	}
}

void GameCollection::doScanGames()
{
	// Load gamelist
	const auto gameListPath = dir / "gamelist.xml";
	if (Path::exists(gameListPath)) {
		esGameList = std::make_shared<ESGameList>(gameListPath);
	}

	// Load metadata
	for (auto& e: entries) {
		collectEntryData(e);
	}

	// Sort and index
	std::sort(entries.begin(), entries.end());
	nameIndex.clear();
	for (size_t i = 0; i < entries.size(); ++i) {
		const auto& e = entries[i];
		for (auto& file: e.files) {
			fileIndex[file.toString()] = i;
		}
		nameIndex[e.sortName] = i;
	}	
}

bool GameCollection::isReady() const
{
	return scanningFuture.isReady();
}

void GameCollection::whenReady(std::function<void()> f)
{
	if (scanningFuture.isReady()) {
		f();
	} else {
		scanningFuture.then(Executors::getMainUpdateThread(), [f = std::move(f)](bool)
		{
			f();
		});
	}
}

size_t GameCollection::getNumEntries() const
{
	return entries.size();
}

gsl::span<const GameCollection::Entry> GameCollection::getEntries() const
{
	waitForLoad();
	return entries;
}

const GameCollection::Entry* GameCollection::findEntry(const String& file) const
{
	waitForLoad();
	const auto iter = fileIndex.find(file);
	if (iter != fileIndex.end()) {
		return &entries[iter->second];
	}
	return nullptr;
}

void GameCollection::waitForLoad() const
{
	scanningFuture.wait();
}

void GameCollection::makeEntry(const Path& path)
{
	if (path.getExtension() == ".xml") {
		return;
	}

	auto [cleanName, tags] = parseName(path.replaceExtension("").getFilename().getString());

	const auto iter = nameIndex.find(cleanName);
	if (iter != nameIndex.end()) {
		// Game already listed, merge
		auto& entry = entries[iter->second];
		entry.files.push_back(path);
		entry.sortFiles();
	} else {
		Entry result;
		result.files.push_back(path);
		result.tags = std::move(tags);
		result.sortName = cleanName;

		nameIndex[cleanName] = entries.size();
		entries.push_back(std::move(result));
	}
}

void GameCollection::collectEntryData(Entry& result)
{
	// Try reading from EmulationStation gamelist.xml
	if (esGameList) {
		const auto& path = result.files.front();
		if (const auto* gameListData = esGameList->findData(path.toString())) {
			result.sortName = postProcessSortName(gameListData->name);
			result.displayName = postProcessDisplayName(gameListData->name);
			result.date = gameListData->releaseDate;
			result.description = gameListData->desc;
			result.developer = gameListData->developer;
			result.publisher = gameListData->publisher;
			result.genre = gameListData->genre;
			result.nPlayers = gameListData->players;

			auto tryAdd = [&](MediaType type, const String& str)
			{
				if (!str.isEmpty()) {
					result.media[type] = (dir / str).getString();
				}
			};

			tryAdd(MediaType::Screenshot, gameListData->image);
			tryAdd(MediaType::BoxFront, gameListData->thumbnail);
			tryAdd(MediaType::BoxBack, gameListData->boxback);
			tryAdd(MediaType::Logo, gameListData->marquee);
			tryAdd(MediaType::Manual, gameListData->manual);
			tryAdd(MediaType::Video, gameListData->video);

			return;
		}
	}

	// Fallback
	result.nPlayers = Range<int>(0, 0);
	result.displayName = postProcessDisplayName(result.sortName);
	collectMediaData(result);
}

std::pair<String, Vector<String>> GameCollection::parseName(const String& name)
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

String GameCollection::postProcessSortName(const String& name)
{
	if (name.startsWith("The ")) {
		return name.substr(4);
	}
	return name;
}

String GameCollection::postProcessDisplayName(const String& name)
{
	if (name.contains(", The")) {
		return "The " + name.replaceOne(", The", "");
	}
	return name;
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
	tryAdd(MediaType::Screenshot, imageDir / (gameName + "-image.jpg"));
	tryAdd(MediaType::BoxFront, imageDir / (gameName + "-thumb.png"));
	tryAdd(MediaType::BoxFront, imageDir / (gameName + "-thumb.jpg"));
	tryAdd(MediaType::BoxBack, imageDir / (gameName + "-boxback.png"));
	tryAdd(MediaType::BoxBack, imageDir / (gameName + "-boxback.jpg"));
	tryAdd(MediaType::Logo, imageDir / (gameName + "-marquee.png"));
}
