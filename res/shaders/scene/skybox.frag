#version 450

layout(set = 0, binding = 0) uniform samplerCube in_cubemap;

layout(location = 0) in vec3 in_v_dir;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(in_cubemap, normalize(in_v_dir));
}
