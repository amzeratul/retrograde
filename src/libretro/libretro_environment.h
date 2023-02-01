#pragma once

#include <halley.hpp>
using namespace Halley;

class LibretroEnvironment {
public:
	LibretroEnvironment(String rootDir);

	const String& getSystemDir() const;
	const String& getSaveDir() const;
	const String& getCoreDir() const;

private:
	String rootDir;
	String systemDir;
	String saveDir;
	String coreDir;
};
