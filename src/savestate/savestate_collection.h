#pragma once

#include <halley.hpp>
using namespace Halley;
class LibretroCore;
class SaveState;

enum class SaveStateType {
    Suspend,
    QuickSave,
    Permanent
};

namespace Halley {
	template <>
	struct EnumNames<SaveStateType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"suspend",
				"quicksave",
				"permanent"
			}};
		}
	};
}

class SaveStateCollection {
public:
    SaveStateCollection(Path dir, String gameId);

    void setCore(LibretroCore& core);

	void saveGameState(SaveStateType type);
    void loadGameState(SaveStateType type, size_t idx);
    void deleteGameState(SaveStateType type, size_t idx);

    gsl::span<const std::pair<SaveStateType, size_t>> enumerate() const;
    std::optional<SaveState> getSaveState(SaveStateType type, size_t idx) const;

	bool hasSuspendSave() const;
	bool hasAnySave() const;

private:
    Path dir;
    String gameId;
    LibretroCore* core = nullptr;

    Vector<std::pair<SaveStateType, size_t>> existingSaves;

    void scanFileSystem();
    void sortExisting();
    String getFileName(SaveStateType type, size_t idx) const;
};
