#version 450 core

layout(push_constant) uniform PushConstants {
  vec4 color;
  vec2 in_viewport_size;
  float in_time;
} pc;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

vec2 rand2(vec2 st) {
  st = vec2(dot(st, vec2(127.1, 311.7)), dot(st, vec2(269.5, 183.3)));
  return fract(sin(st) * 43758.5453123);
}

float sdCircle(vec2 p, float r) {
  return length(p) - r;
}

float sdSegment(vec2 p, vec2 a, vec2 b) {
  vec2 pa = p - a, ba = b - a;
  float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
  return length(pa - ba * h);
}

vec2 getPos(vec2 id, vec2 goffset) {
  vec2 rand = rand2(id + goffset) * (pc.in_time * 0.8 + 30);
  vec2 p = sin(rand);
  return goffset * 2 + p;
}

float calcLine(vec2 p, vec2 a, vec2 b) {
  float len = length(a - b);
  float d = sdSegment(p, a, b);
  return smoothstep(0.08 * 0.4, 0.08 * 0.1, d) * smoothstep(2.0, 0.8, len);
}

void main() {
  vec2 uv = in_uv * pc.in_viewport_size / min(pc.in_viewport_size.x, pc.in_viewport_size.y);
  uv *= 25.0;

  vec2 grid_uv = fract(uv) * 2.0 - 1.0;
  vec2 grid_id = vec2(ivec2(floor(uv)));

  float min_dist = 1000.0;
  vec2 ps[9];
  int idx = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      ps[idx] = getPos(grid_id, vec2(i, j));
      min_dist = min(min_dist, length(ps[idx] - grid_uv));
      idx++;
    }
  }

  float value = 0;

  value += calcLine(grid_uv, ps[4], ps[0]);
  value += calcLine(grid_uv, ps[4], ps[1]);
  value += calcLine(grid_uv, ps[4], ps[2]);
  value += calcLine(grid_uv, ps[4], ps[3]);
  value += calcLine(grid_uv, ps[4], ps[5]);
  value += calcLine(grid_uv, ps[4], ps[6]);
  value += calcLine(grid_uv, ps[4], ps[7]);
  value += calcLine(grid_uv, ps[4], ps[8]);

  value += calcLine(grid_uv, ps[1], ps[3]);
  value += calcLine(grid_uv, ps[1], ps[5]);
  value += calcLine(grid_uv, ps[7], ps[3]);
  value += calcLine(grid_uv, ps[7], ps[5]);

  value += 0.0003 / (min_dist * min_dist);

  out_color = pc.color * value;
  // out_color = pc.color;
  out_color.a = 1.0;

  // if (grid_uv.x > 0.99 || grid_uv.y > 0.99 || grid_uv.x < -0.99 || grid_uv.y < -0.99) {
  //   out_color = pc.color;
  // }
}
