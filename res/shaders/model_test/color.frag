#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_mre;

layout(location = 0) out vec4 out_color;

void main() {
  vec3 n = normalize(in_normal);
  vec3 light_dir = normalize(vec3(0.35, -0.75, 0.55));
  float diffuse = max(dot(n, -light_dir), 0.0);
  float ambient = 0.28;
  float emissive = max(in_mre.z, 0.0);
  vec3 color = clamp(in_color, 0.0, 1.0) * (ambient + diffuse * 0.72 + emissive);
  out_color = vec4(color, 1.0);
}
