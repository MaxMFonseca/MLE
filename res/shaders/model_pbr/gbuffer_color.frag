#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_mre;
layout(location = 3) in vec3 in_world_pos;

layout(set = 0, binding = 0) uniform Material {
  vec4 base_color_factor;
  vec4 emissive_factor;
  vec4 pbr_factors;
} material;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec2 out_normal;
layout(location = 2) out vec4 out_params;
layout(location = 3) out vec4 out_emissive;

vec2 signNotZero(vec2 v) {
  return vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

vec2 encodeNormal(vec3 n) {
  n = normalize(n);
  n /= abs(n.x) + abs(n.y) + abs(n.z);
  vec2 encoded = n.xy;
  if (n.z < 0.0) {
    encoded = (1.0 - abs(encoded.yx)) * signNotZero(encoded);
  }
  return encoded * 0.5 + 0.5;
}

void main() {
  vec3 albedo = clamp(in_color * material.base_color_factor.rgb, 0.0, 1.0);
  float alpha = material.base_color_factor.a;
  float metallic = clamp(in_mre.x * material.pbr_factors.x, 0.0, 1.0);
  float roughness = clamp(in_mre.y * material.pbr_factors.y, 0.04, 1.0);
  vec3 emissive = albedo * max(in_mre.z, 0.0) + material.emissive_factor.rgb;

  out_albedo = vec4(albedo, alpha);
  out_normal = encodeNormal(in_normal);
  out_params = vec4(metallic, roughness, 1.0, alpha);
  out_emissive = vec4(emissive, 1.0);
}
