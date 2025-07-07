#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 ini_model_row0;
layout(location = 4) in vec4 ini_model_row1;
layout(location = 5) in vec4 ini_model_row2;
layout(location = 6) in vec4 ini_model_row3;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;

layout(push_constant) uniform PushConstants {
    mat4 vp;
} pc;

void main() {
    mat4 model = mat4(
        ini_model_row0,
        ini_model_row1,
        ini_model_row2,
        ini_model_row3
    );

    gl_Position = pc.vp * model * vec4(in_position, 1.0);
    out_color = in_color;
    out_normal = (in_normal + 1) / 2;
}
