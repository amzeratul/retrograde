#include "es_gamelist.h"

ESGameList::ESGameList(const Path& path)
{
	load(path);
}

const ESGameList::Entry* ESGameList::findData(const String& filePath)
{
	const auto iter = entries.find(filePath);
	if (iter != entries.end()) {
		return &iter->second;
	}
	return nullptr;
}

void ESGameList::load(const Path& path)
{
	// TODO
}
