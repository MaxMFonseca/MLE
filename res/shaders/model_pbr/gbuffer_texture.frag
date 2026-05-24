#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in float in_tangent_w;
layout(location = 4) in vec3 in_world_pos;

layout(set = 0, binding = 0) uniform Material {
  vec4 base_color_factor;
  vec4 emissive_factor;
  vec4 pbr_factors;
} material;

layout(set = 0, binding = 2) uniform sampler2D base_color_tex;
layout(set = 0, binding = 3) uniform sampler2D metallic_roughness_tex;
layout(set = 0, binding = 4) uniform sampler2D normal_tex;
layout(set = 0, binding = 5) uniform sampler2D occlusion_tex;
layout(set = 0, binding = 6) uniform sampler2D emissive_tex;

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
  vec4 base = texture(base_color_tex, in_uv) * material.base_color_factor;
  vec3 n = normalize(in_normal);
  vec3 t = normalize(in_tangent - n * dot(n, in_tangent));
  vec3 b = normalize(cross(n, t) * in_tangent_w);
  mat3 tbn = mat3(t, b, n);

  vec3 normal_sample = texture(normal_tex, in_uv).xyz * 2.0 - 1.0;
  normal_sample.xy *= material.pbr_factors.z;
  vec3 final_normal = normalize(tbn * normal_sample);

  vec4 metallic_roughness = texture(metallic_roughness_tex, in_uv);
  float metallic = clamp(metallic_roughness.b * material.pbr_factors.x, 0.0, 1.0);
  float roughness = clamp(metallic_roughness.g * material.pbr_factors.y, 0.04, 1.0);
  float occlusion = mix(1.0, texture(occlusion_tex, in_uv).r, material.pbr_factors.w);
  vec3 emissive = texture(emissive_tex, in_uv).rgb * material.emissive_factor.rgb;

  out_albedo = vec4(clamp(base.rgb, 0.0, 1.0), base.a);
  out_normal = encodeNormal(final_normal);
  out_params = vec4(metallic, roughness, occlusion, base.a);
  out_emissive = vec4(emissive, 1.0);
}
