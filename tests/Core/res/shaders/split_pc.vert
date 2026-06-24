#version 450 core

layout(push_constant) uniform VertexPC
{
    mat4 transform;
} pc;

vec2 vertices[] = {
    vec2(-0.5, -0.5),
    vec2( 0.5, -0.5),
    vec2( 0.0,  0.5)
};

void main()
{
    gl_Position = pc.transform * vec4(vertices[gl_VertexIndex], 0.0, 1.0);
}
