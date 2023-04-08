#include "es_gamelist.h"

#include "tinyxml/ticpp.h"

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

namespace {
	Date parseDate(String str)
	{
		Date result;
		result.year = str.substr(0, 4).toInteger();
		result.month = str.substr(4, 2).toInteger();
		result.day = str.substr(6, 2).toInteger();
		return result;
	}

	Range<int> parsePlayers(String str)
	{
		Range<int> result;
		if (str.contains('-')) {
			const auto split = str.split('-');
			result.start = split[0].toInteger();
			result.end = split[1].toInteger();
		} else {
			result.start = result.end = str.toInteger();
		}
		return result;
	}

	ESGameList::Entry parseGameNode(const ticpp::Element* gameElement) {
		ESGameList::Entry game;

		for (const ticpp::Element* child = gameElement->FirstChildElement(); child; child = child->NextSiblingElement(false)) {
			std::string childName = child->Value();
			std::string childText = child->GetTextOrDefault("");

			if (childName == "path") game.path = childText;
			else if (childName == "name") game.name = childText;
			else if (childName == "desc") game.desc = childText;
			else if (childName == "image") game.image = childText;
			else if (childName == "video") game.video = childText;
			else if (childName == "marquee") game.marquee = childText;
			else if (childName == "thumbnail") game.thumbnail = childText;
			else if (childName == "fanart") game.fanart = childText;
			else if (childName == "manual") game.manual = childText;
			else if (childName == "boxback") game.boxback = childText;
			else if (childName == "rating") game.rating = std::stof(childText);
			else if (childName == "releasedate") game.releaseDate = parseDate(childText);
			else if (childName == "developer") game.developer = childText;
			else if (childName == "publisher") game.publisher = childText;
			else if (childName == "genre") game.genre = childText;
			else if (childName == "family") game.family = childText;
			else if (childName == "players") game.players = parsePlayers(childText);
			else if (childName == "hidden") game.hidden = (childText == "1");
			else if (childName == "kidgame") game.kidGame = (childText == "1");
			else if (childName == "lastplayed") game.lastPlayed = parseDate(childText);
			else if (childName == "md5") game.md5 = childText;
			else if (childName == "lang") game.lang = childText;
			else if (childName == "region") game.region = childText;
		}

		return game;
	}
}

void ESGameList::load(const Path& path)
{
	const auto fileData = Path::readFileString(path);
	if (fileData.isEmpty()) {
		return;
	}

	ticpp::Document doc;
	doc.Parse(fileData);
	ticpp::Element* root = doc.FirstChildElement("gameList");
    if (!root) {
        return;
    }

    for (const auto* gameElement = root->FirstChildElement("game"); gameElement; gameElement = gameElement->NextSiblingElement("game", false)) {
        auto e = parseGameNode(gameElement);
        auto path = e.path;
		if (path.startsWith("./")) {
			path = path.substr(2);
		}
        entries[path] = std::move(e);
    }
}
