#include "core_config.h"

CoreConfig::CoreConfig(const ConfigNode& node)
{
	id = node["id"].asString();
	if (node.hasKey("options")) {
		options = node["options"].asHashMap<String, String>();
	}
	blockedExtensions = node["blockedExtensions"].asVector<String>({});
	multithreadedLoading = node["multithreadedLoading"].asBool(true);
}

const String& CoreConfig::getId() const
{
	return id;
}

const HashMap<String, String>& CoreConfig::getOptions() const
{
	return options;
}

Vector<String> CoreConfig::filterExtensions(Vector<String> reportedByCore) const
{
	if (!blockedExtensions.empty()) {
		std_ex::erase_if(reportedByCore, [&](const String& ext) { return std_ex::contains(blockedExtensions, ext); });
	}
	return reportedByCore;
}

const Vector<String>& CoreConfig::getBlockedExtensions() const
{
	return blockedExtensions;
}

bool CoreConfig::hasMultithreadedLoading() const
{
	return multithreadedLoading;
}
