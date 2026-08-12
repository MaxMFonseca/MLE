#version 450

layout(location = 0) in vec3 in_pos;

layout(push_constant) uniform PC {
  mat4 mvp;
  vec4 color;
} pc;

layout(location = 0) out vec4 out_color;

void main() {
  out_color = pc.color;
  gl_Position = pc.mvp * vec4(in_pos, 1.0);
}
