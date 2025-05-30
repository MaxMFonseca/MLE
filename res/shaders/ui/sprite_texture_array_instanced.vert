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
layout(location = 2) in vec4 in_color;
layout(location = 3) in vec2 in_texture_offset;
layout(location = 4) in vec2 in_texture_size;
layout(location = 5) in uint in_texture_index;

layout(location = 0) flat out vec4 out_color;
layout(location = 1) flat out uint out_texture_index;
layout(location = 2) out vec2 out_uv;

void main() {
    gl_Position.x = (in_pos.x + (in_size.x * vertices[gl_VertexIndex].x)) * 2. - 1.;
    gl_Position.y = (in_pos.y + (in_size.y * vertices[gl_VertexIndex].y)) * 2. - 1.;
    out_uv = in_texture_offset + (in_texture_size * vertices[gl_VertexIndex]);

    out_color = in_color;
    out_texture_index = in_texture_index;

    //gl_Position.x = vertices[gl_VertexIndex].x;
    //gl_Position.y = vertices[gl_VertexIndex].y;
}
