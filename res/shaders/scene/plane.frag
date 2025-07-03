#version 450

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_material;

layout(push_constant) uniform PushConstants {
  layout(offset = 80)
    vec3 color;
} pc;

void main() {
    out_albedo = vec4(pc.color, 1.0);
    out_normal = vec4(0.0, 1.0, 0.0, 0.0);
    out_material = vec4(0.0, 0.0, 0.0, 1.0);
}
