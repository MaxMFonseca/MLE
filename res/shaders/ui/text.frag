#version 450 core

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PC {
  vec4 color;
  vec4 border_color;
  float border_thickness;
  float text_thickness;
} pc;

void main()
{
  float value = texture(in_color, in_uv).r;
  bool in_text = value >= (1 - pc.text_thickness);
  bool in_border = value >= (1 - pc.text_thickness - pc.border_thickness);
  out_color = in_text ? pc.color : (in_border ? pc.border_color : vec4(0.0));
}
