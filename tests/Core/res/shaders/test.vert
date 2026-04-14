#version 450 core

vec2 vertices[] = {
    vec2( 0.0,  0.0),
    vec2( 1.0,  0.0),
    vec2( 0.0,  1.0),
    vec2( 1.0,  1.0)
};

layout(location = 0) in vec2 ini_pos;
layout(location = 1) in vec2 ini_size;
layout(location = 2) in vec4 ini_color;
layout(location = 3) in vec4 ini_outline_color;
layout(location = 4) in vec2 ini_texture_offset;
layout(location = 5) in vec2 ini_texture_size;
layout(location = 6) in uint ini_texture_index;
layout(location = 7) in float ini_outline_thickness;

layout(location = 0) flat out vec4 out_color;
layout(location = 1) flat out uint out_texture_index;
layout(location = 2) out vec2 out_uv;
layout(location = 3) flat out vec4 out_outline_color;
layout(location = 4) flat out float out_outline_thickness;

void main() {
    gl_Position.x = (ini_pos.x + (ini_size.x * vertices[gl_VertexIndex].x)) * 2. - 1.;
    gl_Position.y = (ini_pos.y + (ini_size.y * vertices[gl_VertexIndex].y)) * 2. - 1.;
    out_uv = ini_texture_offset + (ini_texture_size * vertices[gl_VertexIndex]);

    out_color = ini_color;
    out_texture_index = ini_texture_index;
    out_outline_color = ini_outline_color;
    out_outline_thickness = 0.5 - ini_outline_thickness / 32.0;
}

