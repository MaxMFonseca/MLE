#version 450 core

vec2 vertices[] = {
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0)
    };

#instance 0
layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_size;

layout(location = 0) out vec2 out_uv;

void main() {
    gl_Position.x = (in_pos.x + (in_size.x * vertices[gl_VertexIndex].x)) * 2. - 1.;
    gl_Position.y = (in_pos.y + (in_size.y * vertices[gl_VertexIndex].y)) * 2. - 1.;
    out_uv = vertices[gl_VertexIndex];
}
