#version 450

layout(set = 0, binding = 0) uniform sampler2D in_img;

layout(push_constant) uniform PC {
  vec2 in_offset;
  vec2 in_size;
  float opacity;
} pc;

layout(location = 0) in vec2 in_frag_uv;

layout(location = 0) out vec4 out_color;

void main() {
  vec2 uv = pc.in_offset;
  uv += in_frag_uv / pc.in_size;
  out_color = texture(in_img, uv);
  out_color.a *= pc.opacity;
}
