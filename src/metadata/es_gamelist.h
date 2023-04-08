#pragma once

#include <halley.hpp>
#include "src/config/system_config.h"
using namespace Halley;

class ESGameList {
public:
	struct Entry {
		String path;
		String name;
		String desc;
		String image;
		String video;
		String marquee;
		String thumbnail;
		String fanart;
		String manual;
		String boxback;
		float rating = 0;
		Date releaseDate;
		String developer;
		String publisher;
		String genre;
		String family;
		int minPlayers = 0;
		int maxPlayers = 0;
		bool hidden = false;
		String md5;
		String lang;
		String region;
	};

    ESGameList(const Path& path);

	const Entry* findData(const String& filePath);

private:
	HashMap<String, Entry> entries;

	void load(const Path& path);
};
