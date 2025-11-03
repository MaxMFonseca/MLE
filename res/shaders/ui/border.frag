#version 450

layout(push_constant) uniform pushconstants {
  vec4 color;
  ivec4 rounding_corners_radius_px; // lt, rt, lb, rb
  ivec4 border_thickness_px; // t, b, l, r
  ivec2 viewport_size_px;
} pc;

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
  vec2 size = vec2(pc.viewport_size_px);
  vec2 uv = in_frag_uv;
  vec2 p = uv * size;

  float c1 = step(uv.y, uv.x);
  float c2 = step(1.0, uv.x + uv.y);

  vec4 bt_tblr = pc.border_thickness_px;
  float bt_q = mix(
      mix(bt_tblr.z, bt_tblr.y, c2),
      mix(bt_tblr.x, bt_tblr.w, c2),
      c1
    );

  vec4 r = vec4(pc.rounding_corners_radius_px);
  vec4 r2 = r + bt_q;

  vec2 inside_p = vec2(p.x - bt_tblr.z, p.y - bt_tblr.x);
  vec2 inside_size = vec2(size.x - bt_tblr.z - bt_tblr.w, size.y - bt_tblr.x - bt_tblr.y);
  float inside = inside_p.x >= 0.0 && inside_p.y >= 0.0 && inside_p.x <= inside_size.x && inside_p.y <= inside_size.y ? 1.0 : 0.0;

  float internal = roundedRectCoverage(inside_p, inside_size, r) * inside;
  float border = roundedRectCoverage(p, size, r2);

  out_color = pc.color;

  out_color.a *= border * (1 - internal);
}
