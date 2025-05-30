#version 450 core

vec2 vertices[] = {
        vec2(-1.0, -1.0),
        vec2(3.0, -1.0),
        vec2(-1.0, 3.0)
    };

void main() {
    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
}
