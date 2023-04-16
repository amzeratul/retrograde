#include "savestate_collection.h"
#include <filesystem>
#include "savestate.h"
#include "src/libretro/libretro_core.h"

SaveStateCollection::SaveStateCollection(Path dir, String gameId)
	: dir(std::move(dir))
	, gameId(std::move(gameId))
{
	scanFileSystem();
}

void SaveStateCollection::setCore(LibretroCore& core)
{
	this->core = &core;
}

namespace {
	static uint64_t getCurrentTimestamp() {
	    const auto now = std::chrono::system_clock::now();
	    const auto duration = now.time_since_epoch();
	    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	}
}

Future<std::optional<SaveState>> SaveStateCollection::saveGameState(SaveStateType type)
{
	if (!core) {
		throw Exception("Core not set", 0);
	}

	auto data = core->saveState(LibretroCore::SaveStateType::Normal);
	auto screenshot = std::shared_ptr<Image>(core->getLastScreenImage());
	const auto aspectRatio = core->getSystemAVInfo().aspectRatio;
	const auto rotation = static_cast<uint8_t>(core->getSystemAVInfo().rotation);

	size_t idx = 0;
	if (type == SaveStateType::Permanent) {
		for (const auto& e: existingSaves) {
			if (e.first == SaveStateType::Permanent) {
				idx = std::max(idx, e.second + 1);
			}
		}
	}
	if (!std_ex::contains(existingSaves, std::pair(type, idx))) {
		existingSaves.emplace_back(type, idx);
		sortExisting();
	}

	const auto path = dir / getFileName(type, idx);

	uint64_t timestamp = getCurrentTimestamp();
	uint32_t timePlayed = 0; // TODO

	return Concurrent::execute(Executors::getCPU(), [=, data = std::move(data), screenshot = std::move(screenshot)] () -> std::optional<SaveState>
	{
		SaveState saveState;
		saveState.setSaveData(data);
		if (screenshot) {
			saveState.setScreenShot(*screenshot, aspectRatio, rotation);
		}
		saveState.setTimePlayed(timePlayed);
		saveState.setTimeStamp(timestamp);

		const auto dir = path.parentPath().string();
		std::error_code ec;
		if (!std::filesystem::is_directory(dir, ec)) {
			std::filesystem::create_directories(dir, ec);
		}
		const bool ok = Path::writeFile(path, saveState.toBytes());
		if (!ok) {
			Logger::logError("Failed to save file.");
			return std::nullopt;
		}
		return saveState;
	});
}

SaveStateCollection::LoadResult SaveStateCollection::loadGameState(SaveStateType type, size_t idx)
{
	if (!core) {
		throw Exception("Core not set", 0);
	}

	if (const auto state = getSaveState(type, idx)) {
		const bool ok = core->loadState(state->getSaveData());
		return ok ? LoadResult::Success : LoadResult::Failed;
	} else {
		return LoadResult::FileNotFound;
	}
}

void SaveStateCollection::deleteGameState(SaveStateType type, size_t idx)
{
	std_ex::erase(existingSaves, std::pair(type, idx));
	Path::removeFile(dir / getFileName(type, idx));
}

gsl::span<const std::pair<SaveStateType, size_t>> SaveStateCollection::enumerate() const
{
	return existingSaves.span();
}

std::optional<SaveState> SaveStateCollection::getSaveState(SaveStateType type, size_t idx) const
{
	const auto bytes = Path::readFile(dir / getFileName(type, idx));
	if (!bytes.empty()) {
		return SaveState(bytes.byte_span());
	} else {
		Logger::logError("Save state failed to load");
		return {};
	}
}

bool SaveStateCollection::hasSuspendSave() const
{
	return std_ex::contains_if(existingSaves, [&] (const auto& e) { return e.first == SaveStateType::Suspend; });
}

bool SaveStateCollection::hasAnySave() const
{
	return !existingSaves.empty();
}

void SaveStateCollection::scanFileSystem()
{
	existingSaves.clear();
	std::error_code ec;
	for (const auto& e: std::filesystem::directory_iterator(dir.getNativeString().cppStr(), ec)) {
		if (e.is_regular_file()) {
			const auto filename = String(e.path().filename().string());
			if (filename.startsWith(gameId)) {
				if (filename.endsWith(".state")) {
					if (filename.endsWith(".quicksave.state")) {
						existingSaves.emplace_back(SaveStateType::QuickSave, 0);
					} else if (filename.endsWith(".suspend.state")) {
						existingSaves.emplace_back(SaveStateType::Suspend, 0);
					} else {
						const auto split = filename.split(".");
						if (split.size() >= 3 && split[split.size() - 2].startsWith("s")) {
							const auto idx = split[split.size() - 2].substr(1).toInteger();
							existingSaves.emplace_back(SaveStateType::Permanent, idx);
						}
					}
				}
			}
		}
	}
	sortExisting();
}

void SaveStateCollection::sortExisting()
{
	std::sort(existingSaves.begin(), existingSaves.end(), [&] (const auto& a, const auto& b)
	{
		return std::pair(a.first, -static_cast<int>(a.second)) < std::pair(b.first, -static_cast<int>(b.second));
	});
}

String SaveStateCollection::getFileName(SaveStateType type, size_t idx) const
{
	switch (type) {
	case SaveStateType::QuickSave:
		return gameId + ".quicksave.state";
	case SaveStateType::Suspend:
		return gameId + ".suspend.state";
	case SaveStateType::Permanent:
		return gameId + ".s" + toString(idx) + ".state";
	}
	throw Exception("Unknown save type.", 0);
}
