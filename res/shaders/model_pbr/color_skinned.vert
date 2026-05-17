#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_mre;
layout(location = 4) in uvec4 in_joints0;
layout(location = 5) in vec4 in_weights0;

layout(set = 0, binding = 1) readonly buffer Skinning {
  mat4 joint_mats[];
} skinning;

layout(push_constant) uniform PC {
  mat4 model;
  mat4 view_proj;
} pc;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_mre;

void main() {
  mat4 skin =
      in_weights0.x * skinning.joint_mats[int(in_joints0.x)] +
      in_weights0.y * skinning.joint_mats[int(in_joints0.y)] +
      in_weights0.z * skinning.joint_mats[int(in_joints0.z)] +
      in_weights0.w * skinning.joint_mats[int(in_joints0.w)];

  vec4 local_pos = skin * vec4(in_pos, 1.0);
  vec3 local_normal = mat3(skin) * in_normal;
  vec4 world_pos = pc.model * local_pos;

  out_color = in_color;
  out_normal = normalize(mat3(pc.model) * local_normal);
  out_mre = in_mre;

  gl_Position = pc.view_proj * world_pos;
}
