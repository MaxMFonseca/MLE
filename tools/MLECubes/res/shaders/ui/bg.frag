// https://www.youtube.com/watch?v=3CycKKJiwis
#version 450 core

layout(push_constant) uniform PushConstants {
  vec2 canvas_size;
  vec4 color;
  float time;
} pc;

layout(location = 0) out vec4 out_color;

float vec2ToRandom(vec2 st) {
    return fract(sin(dot(st, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec2 vec2ToRandom2(vec2 st) {
    st = vec2(dot(st, vec2(127.1, 311.7)), dot(st, vec2(269.5, 183.3)));
    return fract(sin(st) * 43758.5453123);
}

float distLine(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

float line(vec2 p, vec2 a, vec2 b) {
    return smoothstep(0.06, 0.003, distLine(p, a, b)) * smoothstep(2.4, 1.1, length(a - b));
}

vec2 getPos(vec2 id, vec2 offset) {
    vec2 rand = vec2ToRandom2(id + offset) * (pc.time * 0.8 + 30);
    return offset + sin(rand) * 0.5;
}

void main() {
    vec2 uv = (gl_FragCoord.xy * 2.0 - pc.canvas_size.xy) / pc.canvas_size.y;
    uv *= 7.0;

    vec2 grid_uv = (fract(uv) - 0.5) * 2.0;
    vec2 grid_id = floor(uv);

    vec2 positions[9];
    int index = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            vec2 offset = vec2(i, j);
            positions[index] = getPos(grid_id, offset) * 2;
            index++;
        }
    }

    float m = 0;
    float sparkle_factor = (sin((pc.time + 5 + vec2ToRandom(grid_id))) * 0.5 + 0.5) / 2 + 0.1;
    for (int i = 0; i < 9; i++) {
        m += line(grid_uv, positions[4], positions[i]);
        vec2 j = (positions[i] - grid_uv) * 20.0;
        float sparkle = 1. / dot(j, j);
        m += sparkle * sparkle_factor;
    }

    m += line(grid_uv, positions[1], positions[3]);
    m += line(grid_uv, positions[1], positions[5]);
    m += line(grid_uv, positions[7], positions[3]);
    m += line(grid_uv, positions[7], positions[5]);

    out_color = vec4(vec3(m), 1.0) * pc.color;

    // if (grid_uv.x > 0.99 || grid_uv.y > 0.99 || grid_uv.x < -0.99 || grid_uv.y < -0.99) {
    //     out_color = vec4(1.0);
    // }
}
