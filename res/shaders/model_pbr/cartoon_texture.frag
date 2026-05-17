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

layout(set = 0, binding = 1) uniform Lighting {
  vec4 sun_direction_intensity;
  vec4 sun_color_ambient;
} lighting;

layout(set = 0, binding = 2) uniform sampler2D base_color_tex;
layout(set = 0, binding = 3) uniform sampler2D metallic_roughness_tex;
layout(set = 0, binding = 4) uniform sampler2D normal_tex;
layout(set = 0, binding = 5) uniform sampler2D occlusion_tex;
layout(set = 0, binding = 6) uniform sampler2D emissive_tex;

layout(location = 0) out vec4 out_color;

float toonBand(float value) {
  if (value > 0.82) {
    return 1.0;
  }
  if (value > 0.52) {
    return 0.72;
  }
  if (value > 0.24) {
    return 0.42;
  }
  return 0.18;
}

void main() {
  vec4 base = texture(base_color_tex, in_uv) * material.base_color_factor;
  vec3 n = normalize(in_normal);
  vec3 t = normalize(in_tangent - n * dot(n, in_tangent));
  vec3 b = normalize(cross(n, t) * in_tangent_w);
  mat3 tbn = mat3(t, b, n);
  vec3 normal_sample = texture(normal_tex, in_uv).xyz * 2.0 - 1.0;
  normal_sample.xy *= material.pbr_factors.z;
  n = normalize(tbn * normal_sample);

  vec3 v = normalize(vec3(0.0, 0.15, 1.0));
  vec3 l = normalize(-lighting.sun_direction_intensity.xyz);
  vec3 h = normalize(v + l);
  vec4 metallic_roughness = texture(metallic_roughness_tex, in_uv);
  float metallic = clamp(metallic_roughness.b * material.pbr_factors.x, 0.0, 1.0);
  float roughness = clamp(metallic_roughness.g * material.pbr_factors.y, 0.0, 1.0);
  float occlusion = mix(1.0, texture(occlusion_tex, in_uv).r, material.pbr_factors.w);
  vec3 emissive = texture(emissive_tex, in_uv).rgb * material.emissive_factor.rgb;

  float ndotl = max(dot(n, l), 0.0);
  float band = toonBand(ndotl);
  float spec = smoothstep(0.82, 0.88, pow(max(dot(n, h), 0.0), mix(64.0, 12.0, roughness)));
  float rim = smoothstep(0.48, 0.82, 1.0 - max(dot(n, v), 0.0)) * 0.24;

  vec3 sun = lighting.sun_color_ambient.rgb * lighting.sun_direction_intensity.w;
  vec3 color = ((base.rgb * (lighting.sun_color_ambient.a + band * sun)) + vec3(spec) * sun * (1.0 - metallic) + rim * sun) * occlusion + emissive;
  color = color / (color + vec3(1.0));
  out_color = vec4(color, base.a);
}
