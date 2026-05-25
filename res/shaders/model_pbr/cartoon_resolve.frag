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
  vec4 toon_levels;
  vec4 toon_params;
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

float toonBand(float value) {
  float s = max(pc.toon_levels.w, 0.001);
  float b1 = smoothstep(0.24 - s, 0.24 + s, value);
  float b2 = smoothstep(0.52 - s, 0.52 + s, value);
  float b3 = smoothstep(0.82 - s, 0.82 + s, value);

  float res = pc.toon_levels.x;
  res = mix(res, 0.42, b1);
  res = mix(res, pc.toon_levels.y, b2);
  res = mix(res, pc.toon_levels.z, b3);
  return res;
}

void main() {
  float depth = texture(depth_tex, in_uv).r;
  if (depth >= 0.999999) {
    out_color = vec4(0.0);
    return;
  }

  vec4 albedo = texture(albedo_tex, in_uv);
  vec3 n = decodeNormal(texture(normal_tex, in_uv).rg);
  vec4 params = texture(params_tex, in_uv);
  vec3 emissive = texture(emissive_tex, in_uv).rgb;
  vec3 world_pos = reconstructWorldPos(depth);

  vec3 v = normalize(lighting_uniform.camera_pos.xyz - world_pos);
  vec3 l = normalize(-lighting_uniform.sun_direction_intensity.xyz);
  vec3 h = normalize(v + l);

  float ndotl = max(dot(n, l), 0.0);
  float band = toonBand(ndotl);
  float spec = smoothstep(0.82, 0.86, pow(max(dot(n, h), 0.0), mix(128.0, 16.0, params.g))) * pc.toon_params.x;
  float rim = smoothstep(0.6, 0.62, 1.0 - max(dot(n, v), 0.0)) * pc.toon_params.y * band;

  vec3 sun = lighting_uniform.sun_color_ambient.rgb * lighting_uniform.sun_direction_intensity.w;
  vec3 color = ((albedo.rgb * (lighting_uniform.sun_color_ambient.a + band * sun)) + vec3(spec) * sun * (1.0 - params.r) + rim * sun) * params.b + emissive;

  out_color = vec4(color, albedo.a);
}
