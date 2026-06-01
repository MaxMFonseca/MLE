#version 450 core

vec2 vertices[] = {
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
  };

layout(location = 0) out vec2 out_uv;

void main() {
  vec2 local_uv = vertices[gl_VertexIndex];
  gl_Position = vec4(local_uv * 2.0 - 1.0, 0.0, 1.0);
  out_uv = local_uv;
}
