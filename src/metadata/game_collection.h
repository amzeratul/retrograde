#pragma once

#include <halley.hpp>
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

        void sortFiles();
        const Path& getBestFileToLoad(const CoreConfig& coreConfig) const;
        const Path& getMedia(MediaType type) const;

        bool operator<(const Entry& other) const;
    };

    GameCollection(Path dir);

    void scanGames();
    gsl::span<const Entry> getEntries() const;

private:
    Path dir;
    Vector<Entry> entries;
    HashMap<String, size_t> index;

    void makeEntry(const Path& path);
    void collectMediaData(Entry& entry);

	static std::pair<String, Vector<String>> parseName(const String& name);
    static String postProcessName(const String& name);
};
