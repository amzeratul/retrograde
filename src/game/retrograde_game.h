#pragma once

#include <halley.hpp>

#include "game_stage.h"
#include "src/libretro/libretro_core.h"
using namespace Halley;

class RetrogradeGame : public Game {
public:
	void init(const Environment& env, const Vector<String>& args) override;
	int initPlugins(IPluginRegistry& registry) override;
	ResourceOptions initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator) override;

	String getName() const override;
	String getDataPath() const override;
	bool isDevMode() const override;
	std::unique_ptr<Stage> startGame() override;

	const Vector<String>& getArgs() const;
	
	double getTargetFPS() const override;
	double getFixedUpdateFPS() const override;
	void setTargetFPS(std::optional<double> fps);

private:
	const HalleyAPI* api;
	Vector<String> args;
	std::optional<double> targetFps;
};
