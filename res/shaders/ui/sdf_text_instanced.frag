#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform texture2D in_texs[];
layout(set = 1, binding = 0) uniform sampler in_sampler;

layout(location = 0) in flat vec4 in_color;
layout(location = 1) in flat uint in_texture_index;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in flat vec4 in_outline_color;
layout(location = 4) in flat float in_outline_width;

layout(location = 0) out vec4 out_color;

void main()
{
    float r = texture(sampler2D(in_texs[in_texture_index], in_sampler), in_uv).r;
    float main = step(0.5, r);
    float outline = (1.0 - main) * step(in_outline_width, r);  
    out_color = in_color * main + in_outline_color * outline;
    // out_color = vec4(1.0);
}
