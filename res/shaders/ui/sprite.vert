#version 450 core

vec2 vertices[] = {
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
  };

layout(push_constant) uniform PushConstants {
  bool flip_x;
  bool flip_y;
  int pad;
  int pad2;
} pc;

layout(location = 0) out vec2 out_uv;

void main() {
  out_uv = vertices[gl_VertexIndex];
  gl_Position = vec4(out_uv * 2.0 - 1., 0.0, 1.0);
  if (pc.flip_x) {
    out_uv.x = 1.0 - out_uv.x;
  }
  if (pc.flip_y) {
    out_uv.y = 1.0 - out_uv.y;
  }
}
