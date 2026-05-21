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
  vec2 uv;
  vec2 uv_size;
  vec2 padding0;
} pc;

layout(location = 0) out vec2 out_uv;

void main() {
  vec2 local_uv = vertices[gl_VertexIndex];
  gl_Position = vec4(local_uv * 2.0 - 1., 0.0, 1.0);
  if (pc.flip_x) {
    local_uv.x = 1.0 - local_uv.x;
  }
  if (pc.flip_y) {
    local_uv.y = 1.0 - local_uv.y;
  }
  out_uv = pc.uv + local_uv * pc.uv_size;
}
