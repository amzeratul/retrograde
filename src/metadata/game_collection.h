#pragma once

#include <halley.hpp>

#include "src/config/system_config.h"

class CoreConfig;
class ESGameList;
using namespace Halley;

class GameCollection {
public:
    enum class MediaType {
	    Screenshot,
        TitleScreen,
        BoxFront,
        BoxBack,
        Logo,
        Video,
        Manual
    };

    struct Entry {
        String sortName;
        String displayName;
        Vector<Path> files;
        Vector<String> tags;
        HashMap<MediaType, Path> media;
        String description;
        String developer;
        String publisher;
        String genre;
        Date date;
        Range<int> nPlayers;
        bool hidden = false;

        void sortFiles();
        const Path& getBestFileToLoad(const CoreConfig& coreConfig) const;
        const Path& getMedia(MediaType type) const;

        bool operator<(const Entry& other) const;
    };

    GameCollection(Path dir);

    void scanGames();
    void scanGameData();

	bool isReady() const;
    void whenReady(std::function<void()> f);

    size_t getNumEntries() const;
	gsl::span<const Entry> getEntries() const;
    const Entry* findEntry(const String& file) const;

private:
    Path dir;
    Vector<Entry> entries;
    HashMap<String, size_t> nameIndex;
    HashMap<String, size_t> fileIndex;
    std::shared_ptr<ESGameList> esGameList;

    bool gameDataRequested = false;
    Future<bool> scanningFuture;

    void doScanGames();
    void waitForLoad() const;

    void makeEntry(const Path& path);
    void collectEntryData(Entry& result);
    void collectMediaData(Entry& entry);

	static std::pair<String, Vector<String>> parseName(const String& name);
    static String postProcessSortName(const String& name);
    static String postProcessDisplayName(const String& name);
};
