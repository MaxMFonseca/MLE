#pragma once

#include <array>
#include <optional>
#include <sol/forward.hpp>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "mle/utils/Types.h"
#include "mle/utils/Utils.h"

namespace mle {
class Lua;
}

namespace mle::user {
enum class ModelTestShaderMode : u8 {
    PBR = 0,
    CARTOON,
    WIREFRAME,
    NORMALS,
    ALBEDO,
    HOLOGRAM,
    COUNT,
};

enum class ModelTestShaderParameterType : u8 {
    FLOAT,
    COLOR,
};

using ModelTestShaderParameterValue = std::variant<f32, vec4f>;

struct ModelTestShaderParameterDescriptor {
    std::string_view id;
    std::string_view display_name;
    ModelTestShaderParameterType type = ModelTestShaderParameterType::FLOAT;
    ModelTestShaderParameterValue default_value = 0.0F;
    ModelTestShaderParameterValue min_value = 0.0F;
    ModelTestShaderParameterValue max_value = 1.0F;
};

enum class ModelTestResolveInputs : u8 {
    LIT,
    DEBUG,
};

struct ModelTestShaderModeDescriptor {
    ModelTestShaderMode mode = ModelTestShaderMode::PBR;
    std::string_view id;
    std::string_view display_name;
    std::span<const ModelTestShaderParameterDescriptor> parameters;
    std::string_view resolve_pipeline;
    std::string_view resolve_fragment_shader;
    ModelTestResolveInputs resolve_inputs = ModelTestResolveInputs::LIT;
    bool needs_outline = false;
    bool wireframe = false;
    bool bypass_tonemap = false;
    bool blend_tonemap = false;
};

struct ModelTestProjectionPipelineDescriptor {
    std::string_view pipeline;
    std::string_view vertex_shader;
    std::string_view fragment_shader;
};

enum class ModelTestCompositionPassKind : u8 {
    RESOLVE,
    PROJECTION,
    TONEMAP,
};

enum class ModelTestCompositionTarget : u8 {
    HDR_SCENE,
    OUTPUT,
};

enum class ModelTestCompositionInput : u8 {
    NONE,
    HDR_SCENE,
};

enum class ModelTestCompositionPipeline : u8 {
    MODE_RESOLVE,
    FLAT_PROJECTION,
    TONEMAP,
};

struct ModelTestCompositionPass {
    ModelTestCompositionPassKind kind = ModelTestCompositionPassKind::RESOLVE;
    ModelTestCompositionTarget target = ModelTestCompositionTarget::HDR_SCENE;
    ModelTestCompositionInput input = ModelTestCompositionInput::NONE;
    ModelTestCompositionPipeline pipeline = ModelTestCompositionPipeline::MODE_RESOLVE;
    ModelTestShaderMode shader_mode = ModelTestShaderMode::PBR;
    bool blend = false;
    bool bypass_tonemap = false;
    vec4f color{0.0F};
};

struct ModelTestCompositionPlan {
    std::array<ModelTestCompositionPass, 3> entries{};
    usize pass_count = 0;

    [[nodiscard]] std::span<const ModelTestCompositionPass> activePasses() const { return {entries.data(), pass_count}; }
};

[[nodiscard]] std::span<const ModelTestShaderModeDescriptor> modelTestShaderModeDescriptors();
[[nodiscard]] const ModelTestShaderModeDescriptor* findModelTestShaderMode(std::string_view id);
[[nodiscard]] const ModelTestShaderModeDescriptor& modelTestShaderModeDescriptor(ModelTestShaderMode mode);
[[nodiscard]] const ModelTestProjectionPipelineDescriptor& modelTestProjectionPipelineDescriptor();

class ModelTestRendererState {
  public:
    ModelTestRendererState();

    bool setMode(std::string_view id);
    bool setParameter(std::string_view mode_id, std::string_view parameter_id, ModelTestShaderParameterValue value);

    [[nodiscard]] ModelTestShaderMode mode() const { return mode_; }
    [[nodiscard]] const ModelTestShaderModeDescriptor& currentDescriptor() const;
    [[nodiscard]] std::span<const ModelTestShaderParameterDescriptor> currentParameterSchema() const;
    [[nodiscard]] std::optional<ModelTestShaderParameterValue> parameterValue(std::string_view mode_id, std::string_view parameter_id) const;
    [[nodiscard]] const std::string& status() const { return status_; }

  private:
    bool fail(std::string message);

    ModelTestShaderMode mode_ = ModelTestShaderMode::PBR;
    std::array<std::vector<ModelTestShaderParameterValue>, as<usize>(ModelTestShaderMode::COUNT)> values_;
    std::string status_;
};

[[nodiscard]] ModelTestCompositionPlan makeModelTestCompositionPlan(const ModelTestRendererState& renderer, bool show_projection, vec4f projection_color_srgb);

struct ModelTestResolveSettings {
    vec4f effect_params_0{0.0F};
    vec4f effect_params_1{0.0F};
    vec4f effect_params_2{0.0F};
    bool blend_tonemap = false;
};

[[nodiscard]] ModelTestResolveSettings makeModelTestResolveSettings(const ModelTestRendererState& renderer, f32 time);

void bindModelTestShaderLua(Lua& lua, sol::table& api, ModelTestRendererState& renderer);
}  // namespace mle::user
