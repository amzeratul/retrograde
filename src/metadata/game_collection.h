#pragma once

#include <halley.hpp>
using namespace Halley;

class GameCollection {
public:
    struct Entry {
        Path filename;
        String displayName;
    };

    GameCollection(Path dir);

    void scanGames();
    gsl::span<const Entry> getEntries() const;

private:
    Path dir;
    Vector<Entry> entries;

    Entry makeEntry(const Path& path) const;
};
