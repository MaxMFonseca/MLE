#version 450 core

vec2 vertices[] = {
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0)
    };

layout(push_constant) uniform PushConstants {
    vec4 color;
} pc;

layout(location = 0) out vec4 out_color;

void main() {
    gl_Position.x = vertices[gl_VertexIndex].x * 2. - 1.;
    gl_Position.y = vertices[gl_VertexIndex].y * 2. - 1.;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
    out_color = pc.color;
}
