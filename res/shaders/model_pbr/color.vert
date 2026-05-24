#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_mre;

layout(push_constant) uniform PC {
  mat4 model;
  mat4 view_proj;
} pc;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_mre;
layout(location = 3) out vec3 out_world_pos;

void main() {
  vec4 world_pos = pc.model * vec4(in_pos, 1.0);

  out_color = in_color;
  out_normal = normalize(mat3(pc.model) * in_normal);
  out_mre = in_mre;
  out_world_pos = world_pos.xyz;

  gl_Position = pc.view_proj * world_pos;
}
