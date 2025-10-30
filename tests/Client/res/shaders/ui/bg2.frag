#version 450 core

layout(push_constant) uniform PushConstants {
  vec4 color;
  vec2 in_viewport_size;
  float in_time;
} pc;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

const float CIRCLE_RADIUS = 0.08;

vec2 rand2(vec2 st) {
  st = vec2(dot(st, vec2(127.1, 311.7)), dot(st, vec2(269.5, 183.3)));
  return fract(sin(st) * 43758.5453123);
}

vec2 getPos(vec2 id, vec2 goffset) {
  vec2 rand = rand2(id + goffset) * (pc.in_time * 0.8 + 30);
  vec2 p = sin(rand);
  p = p * (1.0 - CIRCLE_RADIUS * 2) + CIRCLE_RADIUS;
  return goffset * 2 + p;
}

void main() {
  vec2 uv = in_uv * pc.in_viewport_size / min(pc.in_viewport_size.x, pc.in_viewport_size.y);
  uv *= 5.0;

  vec2 grid_uv = fract(uv) * 2.0 - 1.0;
  vec2 grid_id = floor(uv);

  float min_dist = 1000.0;
  int idx = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      min_dist = min(min_dist, length(getPos(grid_id, vec2(i, j)) - grid_uv));
    }
  }
  float str = 1 - min_dist / 1.3;
  str = clamp(str, 0.0, 1.0);

  out_color = vec4(pc.color.rgb * str, 1.0);
}
