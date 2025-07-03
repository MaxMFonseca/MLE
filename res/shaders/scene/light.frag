#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D in_albedo;
layout(set = 0, binding = 1) uniform sampler2D in_normal;
layout(set = 0, binding = 2) uniform sampler2D in_material;

layout(location = 0) out vec4 out_color;

void main() {
  out_color = vec4(texture(in_albedo, in_uv).xyz, 1);
}

