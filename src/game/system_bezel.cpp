#include "system_bezel.h"

SystemBezel::SystemBezel(const RetrogradeEnvironment& env)
	: env(env)
{
}

void SystemBezel::setBezel(const BezelConfig* config)
{
	// TODO
}

Rect4i SystemBezel::update(Rect4i windowSize)
{
	// TODO
	return windowSize;
}

void SystemBezel::draw(Painter& painter) const
{
	// TODO
}
