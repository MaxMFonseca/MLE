#version 450

layout(set = 0, binding = 0) uniform samplerCube u_cubemap;

layout(location = 0) in vec3 v_dir;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(u_cubemap, normalize(v_dir));
}
