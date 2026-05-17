#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D depth_tex;

layout(push_constant) uniform PC {
  vec2 inv_extent;
  float depth_threshold;
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

void main() {
  float center_raw = texture(depth_tex, in_uv).r;
  if (center_raw >= 0.999999) {
    discard;
  }

  float center = linearDepth01(center_raw);
  vec2 px = pc.inv_extent * max(pc.outline_width_px, 0.5);
  vec2 px_half = px * 0.5;
  float edge = 0.0;
  edge = max(edge, abs(center - linearDepth01(texture(depth_tex, in_uv + vec2(-px.x, 0.0)).r)));
  edge = max(edge, abs(center - linearDepth01(texture(depth_tex, in_uv + vec2(px.x, 0.0)).r)));
  edge = max(edge, abs(center - linearDepth01(texture(depth_tex, in_uv + vec2(0.0, -px.y)).r)));
  edge = max(edge, abs(center - linearDepth01(texture(depth_tex, in_uv + vec2(0.0, px.y)).r)));
  edge = max(edge, abs(center - linearDepth01(texture(depth_tex, in_uv + vec2(-px_half.x, -px_half.y)).r)));
  edge = max(edge, abs(center - linearDepth01(texture(depth_tex, in_uv + vec2(px_half.x, -px_half.y)).r)));
  edge = max(edge, abs(center - linearDepth01(texture(depth_tex, in_uv + vec2(-px_half.x, px_half.y)).r)));
  edge = max(edge, abs(center - linearDepth01(texture(depth_tex, in_uv + vec2(px_half.x, px_half.y)).r)));
  float mask = smoothstep(pc.depth_threshold, pc.depth_threshold * 3.0, edge);
  if (mask <= 0.001) {
    discard;
  }

  out_color = vec4(0.0, 0.0, 0.0, mask * pc.alpha);
}
