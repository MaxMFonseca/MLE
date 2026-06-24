#version 450 core

layout(push_constant) uniform FragmentPC
{
    layout(offset = 64)
    vec4 color;
} pc;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = pc.color;
}
