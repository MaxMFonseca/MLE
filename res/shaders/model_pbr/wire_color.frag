#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_mre;

layout(set = 0, binding = 0) uniform Material {
  vec4 base_color_factor;
  vec4 emissive_factor;
  vec4 pbr_factors;
} material;

layout(location = 0) out vec4 out_color;

void main() {
  vec3 base = clamp(in_color * material.base_color_factor.rgb, 0.0, 1.0);
  vec3 color = mix(vec3(0.02, 0.025, 0.03), base, 0.72) + vec3(0.18, 0.32, 0.48);
  out_color = vec4(clamp(color, 0.0, 1.0), material.base_color_factor.a);
}
