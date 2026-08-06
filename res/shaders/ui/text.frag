#version 450 core

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PC {
  vec4 color;
  vec4 border_color;
  float border_thickness;
  float text_thickness;
} pc;

void main()
{
  float value = texture(in_color, in_uv).r;
  float text_edge = 1.0 - pc.text_thickness;
  float sdf_per_px = max(length(vec2(dFdx(value), dFdy(value))), 1.0 / 255.0);
  float border_edge = text_edge - pc.border_thickness * sdf_per_px;
  float aa = sdf_per_px * 0.5;

  float text_coverage = smoothstep(text_edge - aa, text_edge + aa, value);
  float border_coverage = smoothstep(border_edge - aa, border_edge + aa, value);
  float text_mix = clamp(text_coverage / max(border_coverage, 0.00001), 0.0, 1.0);

  out_color = mix(pc.border_color, pc.color, text_mix);
  out_color.a *= border_coverage;
}
