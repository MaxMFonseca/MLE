#include "ModelTestRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "mle/lua/Lua.h"
#include "mle/utils/Color.h"

namespace mle::user {
namespace {
const std::array<ModelTestShaderParameterDescriptor, 8> CARTOON_PARAMETERS{{
    {.id = "band_softness", .display_name = "Band softness", .default_value = 0.01F, .min_value = 0.001F, .max_value = 0.08F},
    {.id = "shadow_level", .display_name = "Shadow level", .default_value = 0.18F, .min_value = 0.0F, .max_value = 1.5F},
    {.id = "mid_level", .display_name = "Mid level", .default_value = 0.72F, .min_value = 0.0F, .max_value = 1.5F},
    {.id = "highlight_level", .display_name = "Highlight level", .default_value = 1.0F, .min_value = 0.0F, .max_value = 1.5F},
    {.id = "spec_strength", .display_name = "Spec strength", .default_value = 1.0F, .min_value = 0.0F, .max_value = 2.0F},
    {.id = "rim_strength", .display_name = "Rim strength", .default_value = 0.4F, .min_value = 0.0F, .max_value = 2.0F},
    {.id = "outline_width", .display_name = "Outline width", .default_value = 2.5F, .min_value = 0.5F, .max_value = 8.0F},
    {.id = "outline_normal_threshold", .display_name = "Normal threshold", .default_value = 0.08F, .min_value = 0.02F, .max_value = 0.5F},
}};

const std::array<ModelTestShaderParameterDescriptor, 2> WIREFRAME_PARAMETERS{{
    {.id = "line_width", .display_name = "Line width", .default_value = 1.5F, .min_value = 1.0F, .max_value = 8.0F},
    {.id = "color",
     .display_name = "Color",
     .type = ModelTestShaderParameterType::COLOR,
     .default_value = vec4f{1.0F},
     .min_value = vec4f{0.0F},
     .max_value = vec4f{1.0F}},
}};

const std::array<ModelTestShaderParameterDescriptor, 7> HOLOGRAM_PARAMETERS{{
    {.id = "color",
     .display_name = "Color",
     .type = ModelTestShaderParameterType::COLOR,
     .default_value = vec4f{0.0F, 1.0F, 1.0F, 1.0F},
     .min_value = vec4f{0.0F},
     .max_value = vec4f{1.0F}},
    {.id = "scanline_density", .display_name = "Scanline density", .default_value = 80.0F, .min_value = 4.0F, .max_value = 256.0F},
    {.id = "scanline_speed", .display_name = "Scanline speed", .default_value = 1.0F, .min_value = -4.0F, .max_value = 4.0F},
    {.id = "fresnel_power", .display_name = "Fresnel power", .default_value = 3.0F, .min_value = 0.25F, .max_value = 8.0F},
    {.id = "fresnel_intensity", .display_name = "Fresnel intensity", .default_value = 1.25F, .min_value = 0.0F, .max_value = 4.0F},
    {.id = "opacity", .display_name = "Opacity", .default_value = 0.7F, .min_value = 0.05F, .max_value = 1.0F},
    {.id = "noise", .display_name = "Noise", .default_value = 0.12F, .min_value = 0.0F, .max_value = 0.5F},
}};

const std::array<ModelTestShaderModeDescriptor, as<usize>(ModelTestShaderMode::COUNT)> MODE_DESCRIPTORS{{
    {.mode = ModelTestShaderMode::PBR,
     .id = "pbr",
     .display_name = "PBR",
     .parameters = {},
     .resolve_pipeline = "model_test_pbr_resolve",
     .resolve_fragment_shader = "mle/model_pbr/pbr_resolve.frag"},
    {.mode = ModelTestShaderMode::CARTOON,
     .id = "cartoon",
     .display_name = "Cartoon",
     .parameters = CARTOON_PARAMETERS,
     .resolve_pipeline = "model_test_cartoon_resolve",
     .resolve_fragment_shader = "mle/model_pbr/cartoon_resolve.frag",
     .needs_outline = true},
    {.mode = ModelTestShaderMode::WIREFRAME,
     .id = "wireframe",
     .display_name = "Wireframe",
     .parameters = WIREFRAME_PARAMETERS,
     .resolve_pipeline = "model_test_wireframe_resolve",
     .resolve_fragment_shader = "mle/model_pbr/wireframe_resolve.frag",
     .resolve_inputs = ModelTestResolveInputs::DEBUG,
     .wireframe = true,
     .bypass_tonemap = true},
    {.mode = ModelTestShaderMode::NORMALS,
     .id = "normals",
     .display_name = "Normals",
     .parameters = {},
     .resolve_pipeline = "model_test_normals_resolve",
     .resolve_fragment_shader = "mle/model_pbr/normals_resolve.frag",
     .resolve_inputs = ModelTestResolveInputs::DEBUG,
     .bypass_tonemap = true},
    {.mode = ModelTestShaderMode::ALBEDO,
     .id = "albedo",
     .display_name = "Albedo",
     .parameters = {},
     .resolve_pipeline = "model_test_albedo_resolve",
     .resolve_fragment_shader = "mle/model_pbr/albedo_resolve.frag",
     .resolve_inputs = ModelTestResolveInputs::DEBUG,
     .bypass_tonemap = true},
    {.mode = ModelTestShaderMode::HOLOGRAM,
     .id = "hologram",
     .display_name = "Hologram",
     .parameters = HOLOGRAM_PARAMETERS,
     .resolve_pipeline = "model_test_hologram_resolve",
     .resolve_fragment_shader = "mle/model_pbr/hologram_resolve.frag",
     .blend_tonemap = true},
}};

const ModelTestProjectionPipelineDescriptor PROJECTION_PIPELINE_DESCRIPTOR{
    .pipeline = "model_test_projection_flat",
    .vertex_shader = "mle/model_pbr/projection_flat.vert",
    .fragment_shader = "mle/model_pbr/projection_flat.frag",
};

const ModelTestShaderParameterDescriptor* findParameter(const ModelTestShaderModeDescriptor& mode, std::string_view parameter_id, usize* index = nullptr) {
    const auto it = std::ranges::find(mode.parameters, parameter_id, &ModelTestShaderParameterDescriptor::id);
    if (it == mode.parameters.end()) {
        return nullptr;
    }
    if (index != nullptr) {
        *index = as<usize>(std::distance(mode.parameters.begin(), it));
    }
    return &*it;
}

sol::table makeShaderModeNamesTable(Lua& lua) {
    auto table = lua.createTable();
    for (usize i = 0; i < MODE_DESCRIPTORS.size(); ++i) {
        table[i + 1] = std::string{MODE_DESCRIPTORS[i].display_name};
    }
    return table;
}

sol::table makeShaderModesTable(Lua& lua) {
    auto table = lua.createTable();
    for (usize i = 0; i < MODE_DESCRIPTORS.size(); ++i) {
        auto mode = lua.createTable();
        mode["id"] = std::string{MODE_DESCRIPTORS[i].id};
        mode["display_name"] = std::string{MODE_DESCRIPTORS[i].display_name};
        table[i + 1] = mode;
    }
    return table;
}

sol::table makeColorValueTable(Lua& lua, vec4f rgba) {
    const auto hsva = Color{rgba}.toHSVA();
    auto table = lua.createTable();
    table["h"] = hsva.x / 360.0F;
    table["s"] = hsva.y;
    table["v"] = hsva.z;
    table["a"] = hsva.w;
    return table;
}

sol::table makeCurrentShaderDescriptorTable(Lua& lua, const ModelTestRendererState& renderer) {
    auto table = lua.createTable();
    const auto& descriptor = renderer.currentDescriptor();
    table["id"] = std::string{descriptor.id};
    table["display_name"] = std::string{descriptor.display_name};

    auto parameters = lua.createTable();
    for (usize i = 0; i < descriptor.parameters.size(); ++i) {
        const auto& parameter = descriptor.parameters[i];
        auto schema = lua.createTable();
        schema["id"] = std::string{parameter.id};
        schema["display_name"] = std::string{parameter.display_name};
        schema["type"] = parameter.type == ModelTestShaderParameterType::FLOAT ? "float" : "color";
        if (parameter.type == ModelTestShaderParameterType::FLOAT) {
            schema["default"] = std::get<f32>(parameter.default_value);
            schema["min"] = std::get<f32>(parameter.min_value);
            schema["max"] = std::get<f32>(parameter.max_value);
            schema["value"] = std::get<f32>(renderer.parameterValue(descriptor.id, parameter.id).value());
        } else {
            schema["default"] = makeColorValueTable(lua, std::get<vec4f>(parameter.default_value));
            schema["value"] = makeColorValueTable(lua, std::get<vec4f>(renderer.parameterValue(descriptor.id, parameter.id).value()));
        }
        parameters[i + 1] = schema;
    }
    table["parameters"] = parameters;
    return table;
}

bool setModeCompatible(ModelTestRendererState& renderer, std::string_view name) {
    if (findModelTestShaderMode(name) != nullptr) {
        return renderer.setMode(name);
    }
    const auto it = std::ranges::find(MODE_DESCRIPTORS, name, &ModelTestShaderModeDescriptor::display_name);
    if (it != MODE_DESCRIPTORS.end()) {
        return renderer.setMode(it->id);
    }
    return renderer.setMode(name);
}
}  // namespace

std::span<const ModelTestShaderModeDescriptor> modelTestShaderModeDescriptors() {
    return MODE_DESCRIPTORS;
}

const ModelTestShaderModeDescriptor* findModelTestShaderMode(std::string_view id) {
    const auto it = std::ranges::find(MODE_DESCRIPTORS, id, &ModelTestShaderModeDescriptor::id);
    return it == MODE_DESCRIPTORS.end() ? nullptr : &*it;
}

const ModelTestShaderModeDescriptor& modelTestShaderModeDescriptor(ModelTestShaderMode mode) {
    return MODE_DESCRIPTORS.at(as<usize>(mode));
}

const ModelTestProjectionPipelineDescriptor& modelTestProjectionPipelineDescriptor() {
    return PROJECTION_PIPELINE_DESCRIPTOR;
}

ModelTestRendererState::ModelTestRendererState() {
    for (const auto& descriptor : MODE_DESCRIPTORS) {
        auto& values = values_.at(as<usize>(descriptor.mode));
        values.reserve(descriptor.parameters.size());
        for (const auto& parameter : descriptor.parameters) {
            values.push_back(parameter.default_value);
        }
    }
}

bool ModelTestRendererState::setMode(std::string_view id) {
    const auto* descriptor = findModelTestShaderMode(id);
    if (descriptor == nullptr) {
        return fail("Unknown shader mode '" + std::string{id} + "'");
    }
    mode_ = descriptor->mode;
    status_.clear();
    return true;
}

bool ModelTestRendererState::setParameter(std::string_view mode_id, std::string_view parameter_id, ModelTestShaderParameterValue value) {
    const auto* descriptor = findModelTestShaderMode(mode_id);
    if (descriptor == nullptr) {
        return fail("Unknown shader mode '" + std::string{mode_id} + "'");
    }
    if (descriptor->mode != mode_) {
        return fail("Shader mode '" + std::string{mode_id} + "' is not active");
    }

    usize parameter_index = 0;
    const auto* parameter = findParameter(*descriptor, parameter_id, &parameter_index);
    if (parameter == nullptr) {
        return fail("Unknown parameter '" + std::string{parameter_id} + "' for shader mode '" + std::string{mode_id} + "'");
    }
    const bool has_expected_type = (parameter->type == ModelTestShaderParameterType::FLOAT && std::holds_alternative<f32>(value)) ||
                                   (parameter->type == ModelTestShaderParameterType::COLOR && std::holds_alternative<vec4f>(value));
    if (!has_expected_type) {
        return fail("Parameter '" + std::string{parameter_id} + "' has invalid type");
    }

    if (parameter->type == ModelTestShaderParameterType::FLOAT) {
        const f32 number = std::get<f32>(value);
        const f32 minimum = std::get<f32>(parameter->min_value);
        const f32 maximum = std::get<f32>(parameter->max_value);
        if (!std::isfinite(number) || number < minimum || number > maximum) {
            return fail("Parameter '" + std::string{parameter_id} + "' is outside its range");
        }
    } else {
        const auto color = std::get<vec4f>(value);
        const auto minimum = std::get<vec4f>(parameter->min_value);
        const auto maximum = std::get<vec4f>(parameter->max_value);
        for (usize i = 0; i < color.length(); ++i) {
            if (!std::isfinite(color[i]) || color[i] < minimum[i] || color[i] > maximum[i]) {
                return fail("Parameter '" + std::string{parameter_id} + "' is outside its range");
            }
        }
    }

    values_.at(as<usize>(descriptor->mode)).at(parameter_index) = value;
    status_.clear();
    return true;
}

const ModelTestShaderModeDescriptor& ModelTestRendererState::currentDescriptor() const {
    return modelTestShaderModeDescriptor(mode_);
}

std::span<const ModelTestShaderParameterDescriptor> ModelTestRendererState::currentParameterSchema() const {
    return currentDescriptor().parameters;
}

std::optional<ModelTestShaderParameterValue> ModelTestRendererState::parameterValue(std::string_view mode_id, std::string_view parameter_id) const {
    const auto* descriptor = findModelTestShaderMode(mode_id);
    if (descriptor == nullptr) {
        return std::nullopt;
    }
    usize parameter_index = 0;
    if (findParameter(*descriptor, parameter_id, &parameter_index) == nullptr) {
        return std::nullopt;
    }
    return values_.at(as<usize>(descriptor->mode)).at(parameter_index);
}

bool ModelTestRendererState::fail(std::string message) {
    status_ = std::move(message);
    return false;
}

ModelTestCompositionPlan makeModelTestCompositionPlan(const ModelTestRendererState& renderer, bool show_projection, vec4f projection_color_srgb) {
    const auto& descriptor = renderer.currentDescriptor();
    ModelTestCompositionPlan plan;
    plan.entries[plan.pass_count++] = {
        .kind = ModelTestCompositionPassKind::RESOLVE,
        .target = ModelTestCompositionTarget::HDR_SCENE,
        .input = ModelTestCompositionInput::NONE,
        .pipeline = ModelTestCompositionPipeline::MODE_RESOLVE,
        .shader_mode = descriptor.mode,
    };
    if (show_projection) {
        plan.entries[plan.pass_count++] = {
            .kind = ModelTestCompositionPassKind::PROJECTION,
            .target = ModelTestCompositionTarget::OUTPUT,
            .input = ModelTestCompositionInput::NONE,
            .pipeline = ModelTestCompositionPipeline::FLAT_PROJECTION,
            .shader_mode = descriptor.mode,
            .blend = true,
            .color = static_cast<vec4f>(Color{projection_color_srgb}.toLinear()),
        };
    }
    plan.entries[plan.pass_count++] = {
        .kind = ModelTestCompositionPassKind::TONEMAP,
        .target = ModelTestCompositionTarget::OUTPUT,
        .input = ModelTestCompositionInput::HDR_SCENE,
        .pipeline = ModelTestCompositionPipeline::TONEMAP,
        .shader_mode = descriptor.mode,
        .blend = descriptor.blend_tonemap,
        .bypass_tonemap = descriptor.bypass_tonemap,
    };
    return plan;
}

ModelTestResolveSettings makeModelTestResolveSettings(const ModelTestRendererState& renderer, f32 time) {
    const auto& descriptor = renderer.currentDescriptor();
    ModelTestResolveSettings settings{.blend_tonemap = descriptor.blend_tonemap};
    const auto shader_float = [&renderer](std::string_view mode_id, std::string_view parameter_id) {
        return std::get<f32>(renderer.parameterValue(mode_id, parameter_id).value());
    };

    if (descriptor.mode == ModelTestShaderMode::CARTOON) {
        settings.effect_params_0 = vec4f{shader_float("cartoon", "shadow_level"), shader_float("cartoon", "mid_level"),
                                         shader_float("cartoon", "highlight_level"), shader_float("cartoon", "band_softness")};
        settings.effect_params_1 = vec4f{shader_float("cartoon", "spec_strength"), shader_float("cartoon", "rim_strength"), 0.0F, 0.0F};
    } else if (descriptor.mode == ModelTestShaderMode::WIREFRAME) {
        const auto color_srgb = std::get<vec4f>(renderer.parameterValue("wireframe", "color").value());
        settings.effect_params_0 = static_cast<vec4f>(Color{color_srgb}.toLinear());
    } else if (descriptor.mode == ModelTestShaderMode::HOLOGRAM) {
        const auto color_srgb = std::get<vec4f>(renderer.parameterValue("hologram", "color").value());
        settings.effect_params_0 = static_cast<vec4f>(Color{color_srgb}.toLinear());
        settings.effect_params_1 = vec4f{shader_float("hologram", "scanline_density"), shader_float("hologram", "scanline_speed"),
                                         shader_float("hologram", "fresnel_power"), shader_float("hologram", "fresnel_intensity")};
        settings.effect_params_2 = vec4f{shader_float("hologram", "opacity"), shader_float("hologram", "noise"), time, 0.0F};
    }

    return settings;
}

void bindModelTestShaderLua(Lua& lua, sol::table& api, ModelTestRendererState& renderer) {
    api["model_test_shader_mode_names"] = makeShaderModeNamesTable(lua);
    api["model_test_shader_modes"] = makeShaderModesTable(lua);
    api["model_test_set_shader_mode"] = [&renderer](const std::string& name) { return setModeCompatible(renderer, name); };
    api["model_test_set_shader_parameter"] = [&renderer](const std::string& mode_id, const std::string& parameter_id, const sol::object& value) {
        if (value.get_type() == sol::type::number) {
            return renderer.setParameter(mode_id, parameter_id, value.as<f32>());
        }
        if (value.is<Color>()) {
            return renderer.setParameter(mode_id, parameter_id, static_cast<vec4f>(value.as<Color>()));
        }

        const auto* descriptor = findModelTestShaderMode(mode_id);
        const auto* parameter = descriptor == nullptr ? nullptr : findParameter(*descriptor, parameter_id);
        if (parameter != nullptr && parameter->type == ModelTestShaderParameterType::COLOR) {
            return renderer.setParameter(mode_id, parameter_id, 0.0F);
        }
        return renderer.setParameter(mode_id, parameter_id, vec4f{});
    };
    api["model_test_get_shader_descriptor"] = [&lua, &renderer]() { return makeCurrentShaderDescriptorTable(lua, renderer); };
    api["model_test_shader_status"] = [&renderer]() { return renderer.status(); };
}
}  // namespace mle::user
