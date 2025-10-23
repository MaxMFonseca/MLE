#version 450 core

vec2 vertices[] = {
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
  };

layout(location = 0) in vec2 ini_pos;
layout(location = 1) in vec2 ini_size;
layout(location = 2) in vec2 ini_texture_offset;
layout(location = 3) in vec2 ini_texture_size;

layout(location = 0) out vec2 out_uv;

void main() {
  gl_Position = vec4((ini_pos + vertices[gl_VertexIndex] * ini_size) * 2 - 1, 0.0, 1.0);

  out_uv = ini_texture_offset + vertices[gl_VertexIndex] * ini_texture_size;
}
