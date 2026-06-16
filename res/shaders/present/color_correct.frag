#version 450

layout(set = 0, binding = 0) uniform sampler2D in_img;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

void main() {
  vec4 color = texture(in_img, in_uv);
  vec3 rgb = color.rgb / (color.rgb + vec3(1.0));

  out_color = vec4(rgb, color.a);
}
