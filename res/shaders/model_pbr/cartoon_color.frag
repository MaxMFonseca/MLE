#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_mre;

layout(set = 0, binding = 0) uniform Material {
  vec4 base_color_factor;
  vec4 emissive_factor;
  vec4 pbr_factors;
} material;

layout(set = 0, binding = 1) uniform Lighting {
  vec4 sun_direction_intensity;
  vec4 sun_color_ambient;
} lighting;

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
  vec3 albedo = clamp(in_color * material.base_color_factor.rgb, 0.0, 1.0);
  vec3 n = normalize(in_normal);
  vec3 v = normalize(vec3(0.0, 0.15, 1.0));
  vec3 l = normalize(-lighting.sun_direction_intensity.xyz);
  vec3 h = normalize(v + l);

  float ndotl = max(dot(n, l), 0.0);
  float band = toonBand(ndotl);
  float spec = smoothstep(0.82, 0.88, pow(max(dot(n, h), 0.0), mix(64.0, 12.0, clamp(in_mre.y * material.pbr_factors.y, 0.0, 1.0))));
  float rim = smoothstep(0.48, 0.82, 1.0 - max(dot(n, v), 0.0)) * 0.24;

  vec3 sun = lighting.sun_color_ambient.rgb * lighting.sun_direction_intensity.w;
  vec3 lit = albedo * (lighting.sun_color_ambient.a + band * sun);
  vec3 color = lit + vec3(spec) * sun * (1.0 - clamp(in_mre.x * material.pbr_factors.x, 0.0, 1.0)) + rim * sun + material.emissive_factor.rgb;
  color = color / (color + vec3(1.0));
  out_color = vec4(color, material.base_color_factor.a);
}
