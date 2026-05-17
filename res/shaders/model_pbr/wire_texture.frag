#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in float in_tangent_w;

layout(set = 0, binding = 0) uniform Material {
  vec4 base_color_factor;
  vec4 emissive_factor;
  vec4 pbr_factors;
} material;

layout(set = 0, binding = 2) uniform sampler2D base_color_tex;

layout(location = 0) out vec4 out_color;

void main() {
  vec4 base = texture(base_color_tex, in_uv) * material.base_color_factor;
  vec3 color = mix(vec3(0.02, 0.025, 0.03), base.rgb, 0.72) + vec3(0.18, 0.32, 0.48);
  out_color = vec4(clamp(color, 0.0, 1.0), base.a);
}
