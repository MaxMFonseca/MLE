#version 450 core

vec2 vertices[] = {
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
};

layout(location = 0) out vec2 out_uv;

void main() {
    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
    out_uv = (vertices[gl_VertexIndex] + vec2(1.0)) * 0.5;
}
