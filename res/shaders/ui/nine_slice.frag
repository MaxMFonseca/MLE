#version 450

layout(push_constant) uniform PC {
  vec2 uv;
  vec2 uv_size;
  vec2 source_size_px;
  vec2 padding0;
  ivec4 slice_tblr_px;
  vec4 color;
  ivec4 rounding_corners_radius_px;
  ivec2 viewport_size;
} pc;

layout(binding = 0) uniform sampler2D in_texture;

layout(location = 0) in vec2 in_frag_uv;

layout(location = 0) out vec4 out_color;

void clampPair(inout float a, inout float b, float limit) {
  float sum = a + b;
  if (sum > limit && sum > 0.0) {
    float scale = limit / sum;
    a *= scale;
    b *= scale;
  }
}

float mapNineSliceAxis(float pos, float dst_size, float src_size, float src_start, float src_end, float dst_start, float dst_end) {
  float src_mid = max(src_size - src_start - src_end, 0.0001);
  float dst_mid = max(dst_size - dst_start - dst_end, 0.0001);

  if (pos < dst_start) {
    return mix(0.0, src_start, pos / max(dst_start, 0.0001));
  }
  if (pos > dst_size - dst_end) {
    float t = (pos - (dst_size - dst_end)) / max(dst_end, 0.0001);
    return mix(src_size - src_end, src_size, t);
  }

  float t = (pos - dst_start) / dst_mid;
  return src_start + t * src_mid;
}

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
  vec2 dst_size = max(vec2(pc.viewport_size), vec2(1.0));
  vec2 src_size = max(pc.source_size_px, vec2(1.0));

  float src_top = max(float(pc.slice_tblr_px.x), 0.0);
  float src_bottom = max(float(pc.slice_tblr_px.y), 0.0);
  float src_left = max(float(pc.slice_tblr_px.z), 0.0);
  float src_right = max(float(pc.slice_tblr_px.w), 0.0);

  float dst_top = src_top;
  float dst_bottom = src_bottom;
  float dst_left = src_left;
  float dst_right = src_right;

  clampPair(src_left, src_right, src_size.x);
  clampPair(src_top, src_bottom, src_size.y);
  clampPair(dst_left, dst_right, dst_size.x);
  clampPair(dst_top, dst_bottom, dst_size.y);

  vec2 dst_pos = in_frag_uv * dst_size;
  vec2 src_pos;
  src_pos.x = mapNineSliceAxis(dst_pos.x, dst_size.x, src_size.x, src_left, src_right, dst_left, dst_right);
  src_pos.y = mapNineSliceAxis(dst_pos.y, dst_size.y, src_size.y, src_top, src_bottom, dst_top, dst_bottom);

  vec2 sample_uv = pc.uv + (src_pos / src_size) * pc.uv_size;

  float cov = roundedRectCoverage(dst_pos, dst_size, vec4(pc.rounding_corners_radius_px));
  if (cov <= 0.0) {
    discard;
  }

  vec4 color = texture(in_texture, sample_uv) * pc.color;
  color.a *= pc.color.a * cov;

  out_color = color;
}
