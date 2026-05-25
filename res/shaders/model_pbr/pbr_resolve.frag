#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D albedo_tex;
layout(set = 0, binding = 1) uniform sampler2D normal_tex;
layout(set = 0, binding = 2) uniform sampler2D params_tex;
layout(set = 0, binding = 3) uniform sampler2D emissive_tex;
layout(set = 0, binding = 4) uniform sampler2D depth_tex;

layout(set = 0, binding = 5) uniform Lighting {
  vec4 sun_direction_intensity;
  vec4 sun_color_ambient;
  vec4 camera_pos;
} lighting_uniform;

layout(push_constant) uniform PC {
  mat4 inv_view_proj;
} pc;

layout(location = 0) out vec4 out_color;

vec3 decodeNormal(vec2 encoded) {
  vec2 f = encoded * 2.0 - 1.0;
  vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
  float t = clamp(-n.z, 0.0, 1.0);
  n.xy += vec2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
  return normalize(n);
}

vec3 reconstructWorldPos(float depth) {
  vec4 clip = vec4(in_uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
  vec4 world = pc.inv_view_proj * clip;
  return world.xyz / world.w;
}

vec3 shadePbrPreview(vec3 albedo, float metallic, float roughness, vec3 normal, float occlusion, vec3 emissive, vec3 world_pos) {
  vec3 n = normalize(normal);
  vec3 v = normalize(lighting_uniform.camera_pos.xyz - world_pos);
  vec3 l = normalize(-lighting_uniform.sun_direction_intensity.xyz);
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
  vec3 sun = lighting_uniform.sun_color_ambient.rgb * lighting_uniform.sun_direction_intensity.w;
  vec3 ambient = albedo * lighting_uniform.sun_color_ambient.a;
  return (((diffuse + specular) * ndotl * sun) + ambient + (rim * f)) * occlusion + emissive;
}

void main() {
  float depth = texture(depth_tex, in_uv).r;
  if (depth >= 0.999999) {
    out_color = vec4(0.0);
    return;
  }

  vec4 albedo = texture(albedo_tex, in_uv);
  vec3 normal = decodeNormal(texture(normal_tex, in_uv).rg);
  vec4 params = texture(params_tex, in_uv);
  vec3 emissive = texture(emissive_tex, in_uv).rgb;
  vec3 world_pos = reconstructWorldPos(depth);
  vec3 color = shadePbrPreview(albedo.rgb, params.r, params.g, normal, params.b, emissive, world_pos);
  out_color = vec4(color, albedo.a);
}
