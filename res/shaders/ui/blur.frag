#version 450 core

layout(set = 0, binding = 0) uniform sampler2D u_texture;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_texture, 0));
    vec2 uv = in_uv;

    vec3 result = vec3(0.0);
    float sum = 0.0;

    for (int y = -7; y <= 7; ++y) {
        for (int x = -7; x <= 7; ++x) {
            vec2 o = vec2(x, y) * texelSize;
            result += texture(u_texture, uv + o).rgb;
            sum += 1.0;
        }
    }

    out_color = vec4(result / sum, 1.0);
}
