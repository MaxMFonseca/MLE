#version 450 core

vec2 vertices[] = {
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0)
    };

layout(push_constant) uniform PC {
    vec2 in_pos;
    vec2 in_size;
    vec2 in_clamp_pos;
    vec2 in_clamp_size;
};

layout(location = 0) out vec2 out_uv;

void main() {
    vec2 v = vertices[gl_VertexIndex];

    vec2 clamp_in = (1 - v) * vec2(float(in_clamp_pos.x > 0.0), float(in_clamp_pos.y > 0.0));
    vec2 clamp_out = v * vec2(float(in_clamp_pos.x <= 0.0), float(in_clamp_pos.y <= 0.0));

    gl_Position.xy = in_pos + in_size * v;
    gl_Position.xy += clamp_in * in_size * (1 - in_clamp_size);
    gl_Position.xy -= clamp_out * in_size * (1 - in_clamp_size);
    gl_Position.xy = gl_Position.xy * 2.0 - 1.0;

    out_uv = clamp_in * (1 - in_clamp_size) + clamp_out * in_clamp_size.x;
}
