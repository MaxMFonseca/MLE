#version 450 core
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform texture2D in_texs[];
layout(set = 1, binding = 0) uniform sampler in_sampler;

layout(location = 0) in vec2 in_uv;

layout(push_constant) uniform PushConstants {
    layout(offset = 16)
    vec4 color;
    uint tex_idx;
} in_pc;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(sampler2D(in_texs[in_pc.tex_idx], in_sampler), in_uv);
}
