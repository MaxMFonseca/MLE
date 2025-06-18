#version 450 core

vec2 vertices[] = {
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0)
    };


layout(push_constant) uniform PushConstants {
    vec2 pos;
    vec2 size;
} in_pc;

layout(location = 0) out vec2 out_uv;

void main() {
    gl_Position.x = (in_pc.pos.x + (in_pc.size.x * vertices[gl_VertexIndex].x)) * 2. - 1.;
    gl_Position.y = (in_pc.pos.y + (in_pc.size.y * vertices[gl_VertexIndex].y)) * 2. - 1.;
    out_uv = vertices[gl_VertexIndex];
}
