#version 450 core

layout(push_constant) uniform SharedPC
{
    mat4 transform;
} pc;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(pc.transform[0][0], 0.25, 0.5, 1.0);
}
