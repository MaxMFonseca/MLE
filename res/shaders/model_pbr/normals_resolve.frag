#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D albedo_tex;
layout(set = 0, binding = 1) uniform sampler2D normal_tex;
layout(set = 0, binding = 2) uniform sampler2D depth_tex;

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

void main() {
  float keep_pc = pc.inv_view_proj[0][0] * 0.0;
  float depth = texture(depth_tex, in_uv).r;
  if (depth >= 0.999999) {
    out_color = vec4(1.0);
    return;
  }

  vec3 normal = decodeNormal(texture(normal_tex, in_uv).rg);
  out_color = vec4(normal * 0.5 + 0.5 + keep_pc, texture(albedo_tex, in_uv).a);
}
