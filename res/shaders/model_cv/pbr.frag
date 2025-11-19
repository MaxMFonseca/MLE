#version 450

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_mre;

layout(location = 0) out vec4 out_gbuffer0;
layout(location = 1) out vec4 out_gbuffer1;
layout(location = 2) out vec4 out_gbuffer2;

void main() {
  float m = clamp(in_mre.x, 0.0, 1.0);
  float r = clamp(in_mre.y, 0.0, 1.0);
  float e = max(in_mre.z, 0.0);

  vec3 albedo = clamp(in_color, 0.0, 1.0);

  vec3 N = normalize(in_normal);

  vec3 emissive = albedo * e;

  out_gbuffer0 = vec4(albedo, m);
  out_gbuffer1 = vec4(N, r);
  out_gbuffer2 = vec4(emissive, 0.0);
}
