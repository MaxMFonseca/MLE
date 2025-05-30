#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform sampler2D in_texture[];

layout(location = 0) in flat vec4 in_color;
layout(location = 1) in flat uint in_texture_index;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in flat vec4 in_outline_color;
layout(location = 4) in flat float in_outline_width;

layout(location = 0) out vec4 out_color;

void main()
{
    float r = texture(in_texture[in_texture_index], in_uv).r;
    if (r > 0.5) {
        out_color = in_color;
    } else if (r > in_outline_width) {
        out_color = in_outline_color;
    } else {
        discard;
    }
}
