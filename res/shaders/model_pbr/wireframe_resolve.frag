#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D albedo_tex;
layout(set = 0, binding = 1) uniform sampler2D normal_tex;
layout(set = 0, binding = 2) uniform sampler2D depth_tex;

layout(push_constant) uniform PC {
  mat4 inv_view_proj;
} pc;

layout(location = 0) out vec4 out_color;

void main() {
  float keep_pc = pc.inv_view_proj[0][0] * 0.0 + texture(normal_tex, in_uv).r * 0.0;
  float depth = texture(depth_tex, in_uv).r;
  if (depth >= 0.999999) {
    out_color = vec4(0.0);
    return;
  }

  vec4 base = texture(albedo_tex, in_uv);
  vec3 color = mix(vec3(0.02, 0.025, 0.03), base.rgb, 0.72) + vec3(0.18, 0.32, 0.48);
  out_color = vec4(clamp(color + keep_pc, 0.0, 1.0), base.a);
}
