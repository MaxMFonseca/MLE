#version 450 core

layout(binding = 0) uniform UBO {
    vec2 in_clamp_pos;
    vec2 in_clamp_size;
    vec4 in_roundness;
    vec4 in_color;
    float in_aspect_ratio;
    float in_edge_pos;
    float in_edge_thickness;
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

float sdRoundedBox(in vec2 p, in vec2 b, in vec4 r)
{
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x = (p.y > 0.0) ? r.x : r.y;
    vec2 q = abs(p) - b + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

void main() {
    vec2 coords = in_uv * 2.0 - 1.0;
    coords.x *= in_aspect_ratio;
    vec3 c = vec3(0.0);

    float box = sdRoundedBox(coords, vec2((1 - in_edge_pos) * in_aspect_ratio, 1 - (in_edge_pos * in_aspect_ratio)), in_roundness);
    float edge = step(0.0, box) * step(box, in_edge_thickness);

    out_color = in_color * edge;

    float clamped = step(in_clamp_pos.x, in_uv.x) * step(in_uv.x, in_clamp_pos.x + in_clamp_size.x) * step(in_clamp_pos.y, in_uv.y) * step(in_uv.y, in_clamp_pos.y + in_clamp_size.y);
    out_color.a *= clamped;
}
