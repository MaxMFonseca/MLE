#version 450

layout(set = 0, binding = 0) uniform sampler2D in_img;

layout(push_constant) uniform PC {
  int dst_srgb;
} pc;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

vec3 srgbToLinear(vec3 c) {
  bvec3 cutoff = lessThanEqual(c, vec3(0.04045));
  vec3 lower = c / 12.92;
  vec3 higher = pow((c + 0.055) / 1.055, vec3(2.4));
  return mix(higher, lower, cutoff);
}

void main() {
  vec4 color = texture(in_img, in_uv);
  vec3 rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));

  if (pc.dst_srgb != 0) {
    rgb = srgbToLinear(rgb);
  }

  out_color = vec4(rgb, color.a);
}
