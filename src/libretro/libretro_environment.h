#pragma once

#include <halley.hpp>
using namespace Halley;

class LibretroEnvironment {
public:
	LibretroEnvironment(String rootDir);

	const String& getSystemDir() const;
	const String& getSaveDir() const;
	const String& getCoresDir() const;
	const String& getRomsDir() const;

private:
	String rootDir;
	String systemDir;
	String saveDir;
	String coresDir;
	String romsDir;
};
