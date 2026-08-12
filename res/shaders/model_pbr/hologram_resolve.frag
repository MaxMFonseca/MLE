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
  vec4 hologram_color;
  vec4 scanline_fresnel;
  vec4 opacity_noise_time;
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

float hash12(vec2 value) {
  vec3 p = fract(vec3(value.xyx) * 0.1031);
  p += dot(p, p.yzx + 33.33);
  return fract((p.x + p.y) * p.z);
}

void main() {
  float depth = texture(depth_tex, in_uv).r;
  if (depth >= 0.999999) {
    out_color = vec4(0.0);
    return;
  }

  vec3 albedo = texture(albedo_tex, in_uv).rgb;
  vec3 normal = decodeNormal(texture(normal_tex, in_uv).rg);
  vec4 material_params = texture(params_tex, in_uv);
  vec3 emissive = texture(emissive_tex, in_uv).rgb;
  vec3 world_pos = reconstructWorldPos(depth);
  vec3 view_dir = normalize(lighting_uniform.camera_pos.xyz - world_pos);

  float density = pc.scanline_fresnel.x;
  float phase = world_pos.y * density - pc.opacity_noise_time.z * pc.scanline_fresnel.y * 6.2831853;
  float scanline = smoothstep(0.72, 1.0, 0.5 + 0.5 * sin(phase));
  float edge = pow(clamp(1.0 - abs(dot(normal, view_dir)), 0.0, 1.0),
                   pc.scanline_fresnel.z) * pc.scanline_fresnel.w;
  float noise = (hash12(gl_FragCoord.xy + pc.opacity_noise_time.z * 19.0) - 0.5) *
                pc.opacity_noise_time.y;

  float albedo_luma = dot(albedo, vec3(0.2126, 0.7152, 0.0722));
  float emissive_luma = dot(emissive, vec3(0.2126, 0.7152, 0.0722));
  float material_detail = mix(0.72, 1.0, sqrt(clamp(albedo_luma, 0.0, 1.0)));
  material_detail *= mix(0.9, 1.0, material_params.b);

  vec3 tint = clamp(pc.hologram_color.rgb, vec3(0.0), vec3(1.0));
  vec3 dark_body = tint * 0.22 + vec3(0.008, 0.018, 0.025);
  vec3 bright_edge = max(tint, vec3(0.04)) * (1.0 + edge * 1.35);
  float bright_mask = clamp(scanline * 0.75 + edge * 0.7, 0.0, 1.0);
  vec3 color = mix(dark_body, bright_edge, bright_mask) * material_detail;
  color += tint * (emissive_luma * 0.12 + max(noise, -0.08));

  float opacity = pc.opacity_noise_time.x * pc.hologram_color.a;
  float alpha = clamp(opacity * (0.42 + scanline * 0.25 + edge * 0.35 + noise),
                      0.02, 1.0);
  out_color = vec4(max(color, vec3(0.0)), alpha);
}
