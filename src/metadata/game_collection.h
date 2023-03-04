#pragma once

#include <halley.hpp>
class CoreConfig;
using namespace Halley;

class GameCollection {
public:
    struct Entry {
        String displayName;
        Vector<Path> files;
        Vector<String> tags;

        void sortFiles();
        const Path& getBestFileToLoad(const CoreConfig& coreConfig) const;
    };

    GameCollection(Path dir);

    void scanGames();
    gsl::span<const Entry> getEntries() const;

private:
    Path dir;
    Vector<Entry> entries;
    HashMap<String, size_t> index;

    void makeEntry(const Path& path);
    std::pair<String, Vector<String>> parseName(const String& name) const;
};
