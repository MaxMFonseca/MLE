#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_tangent;
layout(location = 4) in uvec4 in_joints0;
layout(location = 5) in vec4 in_weights0;

layout(set = 0, binding = 7) readonly buffer Skinning {
  mat4 joint_mats[];
} skinning;

layout(push_constant) uniform PC {
  mat4 model;
  mat4 view_proj;
} pc;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out float out_tangent_w;

void main() {
  mat4 skin =
      in_weights0.x * skinning.joint_mats[int(in_joints0.x)] +
      in_weights0.y * skinning.joint_mats[int(in_joints0.y)] +
      in_weights0.z * skinning.joint_mats[int(in_joints0.z)] +
      in_weights0.w * skinning.joint_mats[int(in_joints0.w)];

  vec4 local_pos = skin * vec4(in_pos, 1.0);
  vec3 local_normal = mat3(skin) * in_normal;
  vec3 local_tangent = mat3(skin) * in_tangent.xyz;
  vec4 world_pos = pc.model * local_pos;
  mat3 normal_mat = mat3(pc.model);

  out_uv = in_uv;
  out_normal = normalize(normal_mat * local_normal);
  out_tangent = normalize(normal_mat * local_tangent);
  out_tangent_w = in_tangent.w;

  gl_Position = pc.view_proj * world_pos;
}
