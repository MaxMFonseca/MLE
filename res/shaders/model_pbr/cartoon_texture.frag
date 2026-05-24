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

layout(set = 0, binding = 1) uniform Lighting {
  vec4 sun_direction_intensity;
  vec4 sun_color_ambient;
  vec4 camera_pos;
} material_lighting;

layout(set = 0, binding = 2) uniform sampler2D base_color_tex;
layout(set = 0, binding = 3) uniform sampler2D metallic_roughness_tex;
layout(set = 0, binding = 4) uniform sampler2D normal_tex;
layout(set = 0, binding = 5) uniform sampler2D occlusion_tex;
layout(set = 0, binding = 6) uniform sampler2D emissive_tex;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

float toonBand(float value) {
  const float s = 0.01;
  float b1 = smoothstep(0.24 - s, 0.24 + s, value);
  float b2 = smoothstep(0.52 - s, 0.52 + s, value);
  float b3 = smoothstep(0.82 - s, 0.82 + s, value);
  
  float res = 0.18;
  res = mix(res, 0.42, b1);
  res = mix(res, 0.72, b2);
  res = mix(res, 1.0, b3);
  return res;
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

  vec3 v = normalize(material_lighting.camera_pos.xyz - in_world_pos);
  vec3 l = normalize(-material_lighting.sun_direction_intensity.xyz);
  vec3 h = normalize(v + l);
  vec4 metallic_roughness = texture(metallic_roughness_tex, in_uv);
  float metallic = clamp(metallic_roughness.b * material.pbr_factors.x, 0.0, 1.0);
  float roughness = clamp(metallic_roughness.g * material.pbr_factors.y, 0.0, 1.0);
  float occlusion = mix(1.0, texture(occlusion_tex, in_uv).r, material.pbr_factors.w);
  vec3 emissive = texture(emissive_tex, in_uv).rgb * material.emissive_factor.rgb;

  float ndotl = max(dot(n, l), 0.0);
  float band = toonBand(ndotl);
  float spec = smoothstep(0.82, 0.86, pow(max(dot(n, h), 0.0), mix(128.0, 16.0, roughness)));
  float rim = smoothstep(0.6, 0.62, 1.0 - max(dot(n, v), 0.0)) * 0.4 * band;

  vec3 sun = material_lighting.sun_color_ambient.rgb * material_lighting.sun_direction_intensity.w;
  vec3 color = ((base.rgb * (material_lighting.sun_color_ambient.a + band * sun)) + vec3(spec) * sun * (1.0 - metallic) + rim * sun) * occlusion + emissive;
  
  out_color = vec4(color, base.a);
  out_normal = vec4(n * 0.5 + 0.5, base.a);
}
