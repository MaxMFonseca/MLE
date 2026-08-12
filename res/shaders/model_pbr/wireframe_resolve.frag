#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D albedo_tex;
layout(set = 0, binding = 1) uniform sampler2D normal_tex;
layout(set = 0, binding = 2) uniform sampler2D depth_tex;

layout(push_constant) uniform PC {
  mat4 inv_view_proj;
  vec4 wireframe_color;
} pc;

layout(location = 0) out vec4 out_color;

void main() {
  float depth = texture(depth_tex, in_uv).r;
  if (depth >= 0.999999) {
    out_color = vec4(0.0);
    return;
  }

  out_color = pc.wireframe_color;
}
