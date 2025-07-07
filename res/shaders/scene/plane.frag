#version 450

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_material;

layout(push_constant) uniform PushConstants {
    layout(offset = 80)
    vec3 color;
    int plane_divisions;
} pc;


float N21(vec2 p) {
	  vec3 a = fract(vec3(p.xyx) * vec3(812.532, 234.654, 572.234));
    a += dot(a, a.yzx + 65.87);
    return fract((a.x + a.y) * a.z);
}

void main() {
    vec2 plane_divisions = floor(in_uv * pc.plane_divisions) / pc.plane_divisions;
    float fcolor = N21(plane_divisions) * 0.5 + 0.5;

    out_albedo = vec4(fcolor * pc.color, 1.0);
    out_normal = vec4(0.0, 1.0, 0.0, 0.0);
    out_material = vec4(0.0, 0.0, 0.0, 1.0);
}
