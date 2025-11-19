#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_mre;
layout(location = 4) in int in_color_mix;

layout(set = 0, binding = 0) uniform UBO
{
  mat4 proj;
  mat4 view;
  mat4 model;
  vec3 cam_pos;
} ubo;

layout(push_constant) uniform PC {
  vec3 colors[4];
} pc;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_color;
layout(location = 2) out vec3 out_mre;

void main() {
  out_normal = normalize(mat3(ubo.model) * in_normal);

  vec3 color = in_color;

  if (in_color_mix >= 0 && in_color_mix < 4) {
    color = clamp(color * pc.colors[in_color_mix], 0.0, 1.0);
  }

  out_color = color;

  out_mre = in_mre;

  vec4 world_pos4 = ubo.model * vec4(in_pos, 1.0);
  gl_Position = ubo.proj * ubo.view * world_pos4;
}
