#pragma once

#include <halley.hpp>

#include "src/config/system_config.h"

class CoreConfig;
using namespace Halley;

class GameCollection {
public:
    enum class MediaType {
	    Screenshot,
        BoxFront,
        BoxBack,
        Logo
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
        int nPlayers = 0;

        void sortFiles();
        const Path& getBestFileToLoad(const CoreConfig& coreConfig) const;
        const Path& getMedia(MediaType type) const;

        bool operator<(const Entry& other) const;
    };

    GameCollection(Path dir);

    void scanGames();
    gsl::span<const Entry> getEntries() const;
    const Entry* findEntry(const String& file) const;

private:
    Path dir;
    Vector<Entry> entries;
    HashMap<String, size_t> nameIndex;
    HashMap<String, size_t> fileIndex;

    void makeEntry(const Path& path);
    void collectMediaData(Entry& entry);

	static std::pair<String, Vector<String>> parseName(const String& name);
    static String postProcessName(const String& name);
};
