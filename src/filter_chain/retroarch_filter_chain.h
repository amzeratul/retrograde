#pragma once

#include <halley.hpp>

#include "shader_converter.h"
class ShaderConverter;
using namespace Halley;

#include "filter_chain.h"

enum class RetroarchScaleType {
	Source,
	Absolute,
	Viewport
};

enum class RetroarchWrapMode {
	ClampToEdge,
	ClampToBorder,
	Repeat
};

namespace Halley {
	template <>
	struct EnumNames<RetroarchScaleType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"source",
				"absolute",
				"viewport"
			}};
		}
	};

	template <>
	struct EnumNames<RetroarchWrapMode> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"clamp_to_edge",
				"clamp_to_border",
				"repeat"
			}};
		}
	};
}

class RetroarchFilterChain : public FilterChain {
public:
	class Stage {
	public:
		Stage() = default;
		Stage(int idx, const ConfigNode& params, const Path& basePath);

		Path shaderPath;
		String alias;
		bool filterLinear = false;
		bool mipMapInput = false;
		bool floatFramebuffer = false;
		float srgbFramebuffer = false;
		RetroarchWrapMode wrapMode = RetroarchWrapMode::ClampToEdge;
		RetroarchScaleType scaleTypeX = RetroarchScaleType::Source;
		RetroarchScaleType scaleTypeY = RetroarchScaleType::Source;
		Vector2f scale;

		std::shared_ptr<Material> material;
		std::shared_ptr<MaterialDefinition> materialDefinition;
		std::unique_ptr<RenderSurface> renderSurface;
		Vector2i size;

		void loadMaterial(ShaderConverter& converter, VideoAPI& video);
		void applyParams(const ConfigNode& params);
		Vector2i updateSize(Vector2i sourceSize, Vector2i viewPortSize);
	};

	RetroarchFilterChain() = default;
	RetroarchFilterChain(Path path, VideoAPI& video);

	Sprite run(const Sprite& src, RenderContext& rc, Vector2i viewPortSize) override;

private:
	Path path;
	Vector<Stage> stages;
	uint32_t frameNumber = 0;

	static ConfigNode parsePreset(const Path& path);
	static void parsePresetLine(std::string_view str, ConfigNode::MapType& dst);

	void loadStages(const ConfigNode& params, VideoAPI& video);

	void setupStageMaterial(size_t stageIdx, Vector2i viewPortSize);
	void updateParameter(const String& name, Material& material);
	void updateTexture(const String& name, Material& material);
	void drawStage(const Stage& stage, int stageIdx, Painter& painter);
};
