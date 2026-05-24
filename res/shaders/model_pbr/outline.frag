#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D depth_tex;
layout(set = 0, binding = 1) uniform sampler2D normal_tex;

layout(push_constant) uniform PC {
  vec2 inv_extent;
  float depth_threshold;
  float normal_threshold;
  float alpha;
  float outline_width_px;
} pc;

layout(location = 0) out vec4 out_color;

float linearDepth01(float depth) {
  const float near_plane = 0.01;
  const float far_plane = 1000.0;
  float z = depth * 2.0 - 1.0;
  float linear = (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
  return clamp(linear / far_plane, 0.0, 1.0);
}

vec3 decodeNormal(vec2 uv) {
  vec2 f = texture(normal_tex, uv).rg * 2.0 - 1.0;
  vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
  float t = clamp(-n.z, 0.0, 1.0);
  n.xy += vec2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
  return normalize(n);
}

void main() {
  float center_raw = texture(depth_tex, in_uv).r;
  if (center_raw >= 0.999999) {
    discard;
  }

  float center = linearDepth01(center_raw);
  vec3 center_normal = decodeNormal(in_uv);
  vec2 px = pc.inv_extent * max(pc.outline_width_px, 0.5);
  vec2 px_half = px * 0.5;
  float edge = 0.0;
  float normal_edge = 0.0;

  vec2 offsets[8] = vec2[](
    vec2(-px.x, 0.0),
    vec2(px.x, 0.0),
    vec2(0.0, -px.y),
    vec2(0.0, px.y),
    vec2(-px_half.x, -px_half.y),
    vec2(px_half.x, -px_half.y),
    vec2(-px_half.x, px_half.y),
    vec2(px_half.x, px_half.y)
  );

  for (int i = 0; i < 8; ++i) {
    vec2 uv = in_uv + offsets[i];
    float sample_raw = texture(depth_tex, uv).r;
    edge = max(edge, abs(center - linearDepth01(sample_raw)));
    if (sample_raw < 0.999999) {
      normal_edge = max(normal_edge, 1.0 - clamp(dot(center_normal, decodeNormal(uv)), -1.0, 1.0));
    }
  }

  float depth_mask = smoothstep(pc.depth_threshold, pc.depth_threshold * 3.0, edge);
  float normal_mask = smoothstep(pc.normal_threshold, pc.normal_threshold * 2.0, normal_edge);
  float mask = max(depth_mask, normal_mask);
  if (mask <= 0.001) {
    discard;
  }

  out_color = vec4(0.0, 0.0, 0.0, mask * pc.alpha);
}
