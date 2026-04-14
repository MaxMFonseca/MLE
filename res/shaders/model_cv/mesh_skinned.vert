#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_mre;
layout(location = 4) in uvec4 in_joints0;
layout(location = 5) in vec4 in_weights0;
layout(location = 6) in int in_color_mix;

layout(location = 7) in vec4 ini_model0;
layout(location = 8) in vec4 ini_model1;
layout(location = 9) in vec4 ini_model2;
layout(location = 10) in vec4 ini_model3;
layout(location = 11) in int ini_color_mix_ioffset;

layout(set = 0, binding = 0) uniform UBO {
  mat4 proj;
  mat4 view;
} ubo;

layout(set = 0, binding = 1) readonly buffer Colors {
  vec3 colors[];
} colors;

layout(set = 0, binding = 2) readonly buffer Skinning {
  mat4 joint_mats[];
} skinning;

layout(push_constant) uniform PC {
  int num_joints;
} pc;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_color;
layout(location = 2) out vec3 out_mre;

void main() {
  mat4 model = mat4(ini_model0, ini_model1, ini_model2, ini_model3);

  int joint_base = pc.num_joints * gl_InstanceIndex;
  mat4 skin =
    in_weights0.x * skinning.joint_mats[joint_base + int(in_joints0.x)] +
      in_weights0.y * skinning.joint_mats[joint_base + int(in_joints0.y)] +
      in_weights0.z * skinning.joint_mats[joint_base + int(in_joints0.z)] +
      in_weights0.w * skinning.joint_mats[joint_base + int(in_joints0.w)];

  vec4 skinned_pos_local = skin * vec4(in_pos, 1.0);
  vec3 skinned_normal_local = mat3(skin) * in_normal;

  vec4 world_pos4 = model * skinned_pos_local;
  vec3 world_normal = normalize(mat3(model) * skinned_normal_local);

  out_normal = world_normal;

  vec3 color = in_color;
  if (in_color_mix >= 0) {
    color = clamp(color * colors.colors[ini_color_mix_ioffset + in_color_mix], 0.0, 1.0);
  }
  out_color = color;

  out_mre = in_mre;

  gl_Position = ubo.proj * ubo.view * world_pos4;
}
