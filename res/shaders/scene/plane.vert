#version 450

const vec2 vertices[] = {
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0)
};

layout(push_constant) uniform PushConstants {
    mat4 vp;
    vec2 plane_lb;
    vec2 plane_size;
} pc;

layout(location = 0) out vec2 out_uv;

void main() {
    vec2 world_pos = pc.plane_lb + vertices[gl_VertexIndex] * pc.plane_size;
    gl_Position = pc.vp * vec4(world_pos.x, 0, world_pos.y, 1.0);
    out_uv = vertices[gl_VertexIndex];
}
