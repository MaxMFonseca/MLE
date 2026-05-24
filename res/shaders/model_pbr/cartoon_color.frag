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

layout(set = 0, binding = 1) uniform Lighting {
  vec4 sun_direction_intensity;
  vec4 sun_color_ambient;
  vec4 camera_pos;
} material_lighting;

layout(location = 0) out vec4 out_color;

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
  vec3 albedo = clamp(in_color * material.base_color_factor.rgb, 0.0, 1.0);
  vec3 n = normalize(in_normal);
  vec3 v = normalize(material_lighting.camera_pos.xyz - in_world_pos);
  vec3 l = normalize(-material_lighting.sun_direction_intensity.xyz);
  vec3 h = normalize(v + l);

  float ndotl = max(dot(n, l), 0.0);
  float band = toonBand(ndotl);
  float spec = smoothstep(0.82, 0.86, pow(max(dot(n, h), 0.0), mix(128.0, 16.0, clamp(in_mre.y * material.pbr_factors.y, 0.0, 1.0))));
  float rim = smoothstep(0.6, 0.62, 1.0 - max(dot(n, v), 0.0)) * 0.4 * band;

  vec3 sun = material_lighting.sun_color_ambient.rgb * material_lighting.sun_direction_intensity.w;
  vec3 lit = albedo * (material_lighting.sun_color_ambient.a + band * sun);
  vec3 color = lit + vec3(spec) * sun * (1.0 - clamp(in_mre.x * material.pbr_factors.x, 0.0, 1.0)) + rim * sun + material.emissive_factor.rgb;

  out_color = vec4(color, material.base_color_factor.a);
}
