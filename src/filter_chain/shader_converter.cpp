#include "shader_converter.h"
#include "../contrib/glslang/include/glslang_c_interface.h"

namespace {
	const glslang_resource_t& getDefaultResources()
	{
	    static glslang_resource_t resources;
		static bool initialized = false;

		if (!initialized) {
			resources.max_lights = 32;
			resources.max_clip_planes = 6;
			resources.max_texture_units = 32;
			resources.max_texture_coords = 32;
			resources.max_vertex_attribs = 64;
			resources.max_vertex_uniform_components = 4096;
			resources.max_varying_floats = 64;
			resources.max_vertex_texture_image_units = 32;
			resources.max_combined_texture_image_units = 80;
			resources.max_texture_image_units = 32;
			resources.max_fragment_uniform_components = 4096;
			resources.max_draw_buffers = 32;
			resources.max_vertex_uniform_vectors = 128;
			resources.max_varying_vectors = 8;
			resources.max_fragment_uniform_vectors = 16;
			resources.max_vertex_output_vectors = 16;
			resources.max_fragment_input_vectors = 15;
			resources.min_program_texel_offset = -8;
			resources.max_program_texel_offset = 7;
			resources.max_clip_distances = 8;
			resources.max_compute_work_group_count_x = 65535;
			resources.max_compute_work_group_count_y = 65535;
			resources.max_compute_work_group_count_z = 65535;
			resources.max_compute_work_group_size_x = 1024;
			resources.max_compute_work_group_size_y = 1024;
			resources.max_compute_work_group_size_z = 64;
			resources.max_compute_uniform_components = 1024;
			resources.max_compute_texture_image_units = 16;
			resources.max_compute_image_uniforms = 8;
			resources.max_compute_atomic_counters = 8;
			resources.max_compute_atomic_counter_buffers = 1;
			resources.max_varying_components = 60;
			resources.max_vertex_output_components = 64;
			resources.max_geometry_input_components = 64;
			resources.max_geometry_output_components = 128;
			resources.max_fragment_input_components = 128;
			resources.max_image_units = 8;
			resources.max_combined_image_units_and_fragment_outputs = 8;
			resources.max_combined_shader_output_resources = 8;
			resources.max_image_samples = 0;
			resources.max_vertex_image_uniforms = 0;
			resources.max_tess_control_image_uniforms = 0;
			resources.max_tess_evaluation_image_uniforms = 0;
			resources.max_geometry_image_uniforms = 0;
			resources.max_fragment_image_uniforms = 8;
			resources.max_combined_image_uniforms = 8;
			resources.max_geometry_texture_image_units = 16;
			resources.max_geometry_output_vertices = 256;
			resources.max_geometry_total_output_components = 1024;
			resources.max_geometry_uniform_components = 1024;
			resources.max_geometry_varying_components = 64;
			resources.max_tess_control_input_components = 128;
			resources.max_tess_control_output_components = 128;
			resources.max_tess_control_texture_image_units = 16;
			resources.max_tess_control_uniform_components = 1024;
			resources.max_tess_control_total_output_components = 4096;
			resources.max_tess_evaluation_input_components = 128;
			resources.max_tess_evaluation_output_components = 128;
			resources.max_tess_evaluation_texture_image_units = 16;
			resources.max_tess_evaluation_uniform_components = 1024;
			resources.max_tess_patch_components = 120;
			resources.max_patch_vertices = 32;
			resources.max_tess_gen_level = 64;
			resources.max_viewports = 16;
			resources.max_vertex_atomic_counters = 0;
			resources.max_tess_control_atomic_counters = 0;
			resources.max_tess_evaluation_atomic_counters = 0;
			resources.max_geometry_atomic_counters = 0;
			resources.max_fragment_atomic_counters = 8;
			resources.max_combined_atomic_counters = 8;
			resources.max_atomic_counter_bindings = 1;
			resources.max_vertex_atomic_counter_buffers = 0;
			resources.max_tess_control_atomic_counter_buffers = 0;
			resources.max_tess_evaluation_atomic_counter_buffers = 0;
			resources.max_geometry_atomic_counter_buffers = 0;
			resources.max_fragment_atomic_counter_buffers = 1;
			resources.max_combined_atomic_counter_buffers = 1;
			resources.max_atomic_counter_buffer_size = 16384;
			resources.max_transform_feedback_buffers = 4;
			resources.max_transform_feedback_interleaved_components = 64;
			resources.max_cull_distances = 8;
			resources.max_combined_clip_and_cull_distances = 8;
			resources.max_samples = 4;
			resources.max_mesh_output_vertices_nv = 256;
			resources.max_mesh_output_primitives_nv = 512;
			resources.max_mesh_work_group_size_x_nv = 32;
			resources.max_mesh_work_group_size_y_nv = 1;
			resources.max_mesh_work_group_size_z_nv = 1;
			resources.max_task_work_group_size_x_nv = 32;
			resources.max_task_work_group_size_y_nv = 1;
			resources.max_task_work_group_size_z_nv = 1;
			resources.max_mesh_view_count_nv = 4;

			resources.limits.non_inductive_for_loops = 1;
			resources.limits.while_loops = 1;
			resources.limits.do_while_loops = 1;
			resources.limits.general_uniform_indexing = 1;
			resources.limits.general_attribute_matrix_vector_indexing = 1;
			resources.limits.general_varying_indexing = 1;
			resources.limits.general_sampler_indexing = 1;
			resources.limits.general_variable_indexing = 1;
			resources.limits.general_constant_matrix_vector_indexing = 1;

			initialized = false;
		}

	    return resources;
	}
}

ShaderConverter::ShaderConverter()
{
	if (nInstances++ == 0) {
		glslang_initialize_process();
	}
}

ShaderConverter::~ShaderConverter()
{
	if (--nInstances == 0) {
		glslang_finalize_process();
	}
}

String ShaderConverter::convertShader(const String& src, ShaderStage stage, ShaderFormat inputFormat, ShaderFormat outputFormat)
{
	if (inputFormat == outputFormat) {
		return src;
	}

	auto spirvData = convertToSpirv(src, stage, inputFormat);
	if (outputFormat == ShaderFormat::SPIRV) {
		//return spirvSrc;
		Logger::logError("Outputting SPIRV is not implemented");
		return {};
	}

	if (outputFormat == ShaderFormat::HLSL) {
		return convertSpirvToHLSL(spirvData, stage);
	}

	return {};
}

Bytes ShaderConverter::convertToSpirv(const String& src, ShaderStage stage, ShaderFormat inputFormat)
{
	glslang_input_t input;
	input.language = inputFormat == ShaderFormat::GLSL ? GLSLANG_SOURCE_GLSL : GLSLANG_SOURCE_HLSL;
	input.stage = stage == ShaderStage::Pixel ? GLSLANG_STAGE_FRAGMENT : GLSLANG_STAGE_VERTEX;
	input.client = GLSLANG_CLIENT_VULKAN;
	input.client_version = GLSLANG_TARGET_VULKAN_1_3;
	input.target_language = GLSLANG_TARGET_SPV;
	input.target_language_version = GLSLANG_TARGET_SPV_1_6;
	input.code = src.c_str();
	input.default_profile = GLSLANG_CORE_PROFILE;
	input.default_version = 100;
	input.force_default_version_and_profile = false;
	input.forward_compatible = false;
	input.messages = GLSLANG_MSG_DEFAULT_BIT;
	input.resource = &getDefaultResources();

	glslang_shader_t* shader = glslang_shader_create(&input);
	if (!glslang_shader_preprocess(shader, &input)) {
		Logger::logWarning("Preprocessor error: " + String(glslang_shader_get_info_log(shader)));
	}

	if (!glslang_shader_parse(shader, &input)) {
		Logger::logWarning("Parse error: " + String(glslang_shader_get_info_log(shader)));
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
		Logger::logWarning("Link error: " + String(glslang_program_get_info_log(program)));
	}

	glslang_program_SPIRV_generate(program, input.stage);
	if (glslang_program_SPIRV_get_messages(program)) {
		Logger::logDev("SPIRV messages: " + String(glslang_program_SPIRV_get_messages(program)));
	}

	const size_t size = glslang_program_SPIRV_get_size(program);
	Bytes output;
	output.resize(size * 4);
	memcpy(output.data(), glslang_program_SPIRV_get_ptr(program), size * 4);

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	return output;
}

String ShaderConverter::convertSpirvToHLSL(const Bytes& spirvData, ShaderStage stage)
{
	// TODO
	Logger::logError("SPIRV -> HLSL implementation missing");
	return {};
}

std::unique_ptr<Shader> ShaderConverter::loadShader(const String& vertexSrc, const String& pixelSrc, VideoAPI& video)
{
	// Maybe move this to video api directly?
	// TODO
	return {};
}

size_t ShaderConverter::nInstances = 0;
