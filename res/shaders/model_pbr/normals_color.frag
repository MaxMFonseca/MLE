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
  vec3 n = normalize(in_normal);
  out_color = vec4(n * 0.5 + 0.5, material.base_color_factor.a);
}
