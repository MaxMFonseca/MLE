#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_mre;
layout(location = 4) in uvec4 in_joints0;
layout(location = 5) in vec4 in_weights0;
layout(location = 6) in int in_color_mix;

layout(set = 0, binding = 0) uniform UBO
{
  mat4 proj;
  mat4 view;
  mat4 model;
  vec3 cam_pos;
} ubo;

layout(set = 0, binding = 1) readonly buffer Skinning
{
  mat4 joint_mats[];
} skinning;

layout(push_constant) uniform PC {
  vec3 colors[4];
} pc;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_color;
layout(location = 2) out vec3 out_mre;

void main() {
  mat4 skin =
    in_weights0.x * skinning.joint_mats[in_joints0.x] +
      in_weights0.y * skinning.joint_mats[in_joints0.y] +
      in_weights0.z * skinning.joint_mats[in_joints0.z] +
      in_weights0.w * skinning.joint_mats[in_joints0.w];

  vec4 skinned_pos_local = skin * vec4(in_pos, 1.0);
  vec3 skinned_normal_local = mat3(skin) * in_normal;

  vec4 world_pos4 = ubo.model * skinned_pos_local;
  vec3 world_normal = normalize(mat3(ubo.model) * skinned_normal_local);

  out_normal = world_normal;

  vec3 color = in_color;
  if (in_color_mix >= 0 && in_color_mix < 4) {
    color = clamp(color * pc.colors[in_color_mix], 0.0, 1.0);
  }
  out_color = color;

  out_mre = in_mre;

  gl_Position = ubo.proj * ubo.view * world_pos4;
}
