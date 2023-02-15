#pragma once

#include <halley.hpp>
class RewindData;
class RetrogradeEnvironment;
class LibretroCore;
using namespace Halley;

class GameStage : public EntityStage {
public:
	GameStage();
	~GameStage();
	
	void init() override;

	void onVariableUpdate(Time) override;
	void onFixedUpdate(Time) override;
	void onRender(RenderContext&) const override;

private:
	std::shared_ptr<PerformanceStatsView> perfStats;
	std::unique_ptr<RetrogradeEnvironment> env;
	std::unique_ptr<UIRoot> uiRoot;
	std::unique_ptr<UIFactory> uiFactory;
};
