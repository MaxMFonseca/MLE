#version 450

layout(push_constant) uniform PC {
  vec4 color; // rgba
  ivec4 rounding_corners_radius_px; // lt, rt, lb, rb
  ivec2 viewport_size; // w, h
} pc;

layout(binding = 0) uniform sampler2D in_texture;

layout(location = 0) in vec2 in_frag_uv;

layout(location = 0) out vec4 out_color;

float roundedRectCoverage(vec2 p, vec2 size, vec4 r) {
  vec2 half_size = size * 0.5;
  float rmax = min(half_size.x, half_size.y);
  r = clamp(r, vec4(0.0), vec4(rmax));

  bool in_lt = (p.x < r.x) && (p.y < r.x);
  bool in_rt = (p.x > size.x - r.y) && (p.y < r.y);
  bool in_rb = (p.x > size.x - r.w) && (p.y > size.y - r.w);
  bool in_lb = (p.x < r.z) && (p.y > size.y - r.z);

  if (!(in_lt || in_rt || in_rb || in_lb)) {
    return 1.0;
  }

  vec2 c;
  float rad;
  if (in_lt) {
    c = vec2(r.x, r.x);
    rad = r.x;
  } else if (in_rt) {
    c = vec2(size.x - r.y, r.y);
    rad = r.y;
  } else if (in_rb) {
    c = vec2(size.x - r.w, size.y - r.w);
    rad = r.w;
  } else {
    c = vec2(r.z, size.y - r.z);
    rad = r.z;
  }

  float d = length(p - c) - rad;

  float g = fwidth(d);
  return 1.0 - smoothstep(0.0, g, d);
}

void main() {
  vec2 size = vec2(pc.viewport_size);
  vec2 p = in_frag_uv * size;

  vec4 r = vec4(pc.rounding_corners_radius_px);

  float cov = roundedRectCoverage(p, size, r);
  if (cov <= 0.0) {
    discard;
  }

  vec4 color = texture(in_texture, in_frag_uv) * pc.color;
  color.a *= pc.color.a * cov;

  out_color = color;
}
