#version 450 core

layout(push_constant) uniform PC {
    layout(offset = 32)
    vec4 in_color0;
    vec4 in_color1;
    float in_progress;
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 c = mix(in_color0.rgb, in_color1.rgb, in_progress);
    out_color = vec4(c, 1 * float(in_progress > in_uv.x));
}
