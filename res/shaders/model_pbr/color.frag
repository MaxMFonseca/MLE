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

vec3 shadePbrPreview(vec3 albedo, float metallic, float roughness, vec3 normal, vec3 emissive) {
  vec3 n = normalize(normal);
  vec3 v = normalize(vec3(0.0, 0.15, 1.0));
  vec3 l = normalize(vec3(-0.35, 0.75, -0.55));
  vec3 h = normalize(v + l);

  float ndotl = max(dot(n, l), 0.0);
  float ndotv = max(dot(n, v), 0.001);
  float ndoth = max(dot(n, h), 0.0);
  float vdoth = max(dot(v, h), 0.0);
  float a = max(roughness * roughness, 0.045);
  float a2 = a * a;
  float denom = (ndoth * ndoth) * (a2 - 1.0) + 1.0;
  float d = a2 / max(3.14159265 * denom * denom, 0.001);
  float k = (roughness + 1.0);
  k = (k * k) / 8.0;
  float gv = ndotv / (ndotv * (1.0 - k) + k);
  float gl = ndotl / (ndotl * (1.0 - k) + k);
  vec3 f0 = mix(vec3(0.04), albedo, metallic);
  vec3 f = f0 + (1.0 - f0) * pow(1.0 - vdoth, 5.0);
  vec3 specular = (d * gv * gl * f) / max(4.0 * ndotv * ndotl, 0.001);
  vec3 diffuse = (1.0 - f) * albedo * (1.0 - metallic) / 3.14159265;
  return (diffuse + specular) * ndotl + albedo * 0.08 + emissive;
}

void main() {
  vec3 albedo = clamp(in_color * material.base_color_factor.rgb, 0.0, 1.0);
  float metallic = clamp(in_mre.x * material.pbr_factors.x, 0.0, 1.0);
  float roughness = clamp(in_mre.y * material.pbr_factors.y, 0.04, 1.0);
  vec3 emissive = albedo * max(in_mre.z, 0.0) + material.emissive_factor.rgb;
  vec3 color = shadePbrPreview(albedo, metallic, roughness, in_normal, emissive);
  out_color = vec4(color, material.base_color_factor.a);
}
