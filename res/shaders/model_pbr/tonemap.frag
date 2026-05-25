#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D hdr_scene_tex;

layout(push_constant) uniform PC {
  vec4 params;
} pc;

layout(location = 0) out vec4 out_color;

void main() {
  vec4 hdr = texture(hdr_scene_tex, in_uv);
  if (hdr.a <= 0.001) {
    discard;
  }

  vec3 color = max(hdr.rgb, vec3(0.0));
  if (pc.params.y < 0.5) {
    color *= max(pc.params.x, 0.0);
    color = color / (color + vec3(1.0));
  }

  out_color = vec4(clamp(color, 0.0, 1.0), hdr.a);
}
