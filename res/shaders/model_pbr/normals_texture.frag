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
  vec3 n = normalize(in_normal);
  float alpha = texture(base_color_tex, in_uv).a * material.base_color_factor.a;
  out_color = vec4(n * 0.5 + 0.5, alpha);
}
