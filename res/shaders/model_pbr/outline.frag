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

void main() {
  float center = texture(depth_tex, in_uv).r;
  if (center >= 0.9999) {
    discard;
  }

  vec2 px = pc.inv_extent * max(pc.outline_width_px, 0.5);
  vec2 px_half = px * 0.5;
  float edge = 0.0;
  edge = max(edge, abs(center - texture(depth_tex, in_uv + vec2(-px.x, 0.0)).r));
  edge = max(edge, abs(center - texture(depth_tex, in_uv + vec2(px.x, 0.0)).r));
  edge = max(edge, abs(center - texture(depth_tex, in_uv + vec2(0.0, -px.y)).r));
  edge = max(edge, abs(center - texture(depth_tex, in_uv + vec2(0.0, px.y)).r));
  edge = max(edge, abs(center - texture(depth_tex, in_uv + vec2(-px_half.x, -px_half.y)).r));
  edge = max(edge, abs(center - texture(depth_tex, in_uv + vec2(px_half.x, -px_half.y)).r));
  edge = max(edge, abs(center - texture(depth_tex, in_uv + vec2(-px_half.x, px_half.y)).r));
  edge = max(edge, abs(center - texture(depth_tex, in_uv + vec2(px_half.x, px_half.y)).r));
  float mask = smoothstep(pc.depth_threshold, pc.depth_threshold * 3.0, edge);
  if (mask <= 0.001) {
    discard;
  }

  out_color = vec4(0.0, 0.0, 0.0, mask * pc.alpha);
}
