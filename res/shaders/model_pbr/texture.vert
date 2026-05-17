#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_tangent;

layout(push_constant) uniform PC {
  mat4 model;
  mat4 view_proj;
} pc;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out float out_tangent_w;

void main() {
  vec4 world_pos = pc.model * vec4(in_pos, 1.0);
  mat3 normal_mat = mat3(pc.model);

  out_uv = in_uv;
  out_normal = normalize(normal_mat * in_normal);
  out_tangent = normalize(normal_mat * in_tangent.xyz);
  out_tangent_w = in_tangent.w;

  gl_Position = pc.view_proj * world_pos;
}
