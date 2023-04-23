#pragma once

#include <halley.hpp>
class RewindData;
class RetrogradeEnvironment;
class LibretroCore;
using namespace Halley;

class GameStage : public EntityStage {
public:
	GameStage(RetrogradeEnvironment& env);
	~GameStage();
	
	void init() override;

	void onVariableUpdate(Time) override;
	void onFixedUpdate(Time) override;
	void onRender(RenderContext&) const override;

private:
	RetrogradeEnvironment& env;
	std::shared_ptr<PerformanceStatsView> perfStats;
	std::unique_ptr<UIRoot> uiRoot;
	std::unique_ptr<UIFactory> uiFactory;

	float zoomLevel = 1;

	void onUpdate(Time t);
};
