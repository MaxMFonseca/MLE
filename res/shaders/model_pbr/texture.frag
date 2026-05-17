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

vec3 shadePbrPreview(vec3 albedo, float metallic, float roughness, vec3 normal, float occlusion, vec3 emissive) {
  vec3 n = normalize(normal);
  vec3 v = normalize(vec3(0.0, 0.15, 1.0));
  vec3 l = normalize(-lighting.sun_direction_intensity.xyz);
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
  float rim = pow(1.0 - ndotv, 3.0) * 0.08;
  vec3 sun = lighting.sun_color_ambient.rgb * lighting.sun_direction_intensity.w;
  vec3 ambient = albedo * lighting.sun_color_ambient.a;
  return (((diffuse + specular) * ndotl * sun) + ambient + (rim * f)) * occlusion + emissive;
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
  vec3 color = shadePbrPreview(base.rgb, metallic, roughness, final_normal, occlusion, emissive);
  color = color / (color + vec3(1.0));
  out_color = vec4(color, base.a);
}
