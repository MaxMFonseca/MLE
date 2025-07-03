#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;

layout(push_constant) uniform PC {
  layout(offset = 64)
  float metalness;
  float roughness;
  float emissive;
} pc;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_material;

void main() {
    out_albedo = vec4(in_color, 1.0);
    out_normal = vec4(in_normal, 0.0);
    out_material = vec4(pc.metalness, pc.roughness, pc.emissive, 0.0);
}
