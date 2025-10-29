#version 450 core

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main()
{
  float value = texture(in_color, in_uv).r;
  if (value < 0.5) {
    discard;
  }
  out_color = vec4(1.0, 1.0, 1.0, 1.0);
}
