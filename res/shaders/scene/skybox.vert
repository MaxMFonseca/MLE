#version 450

const vec3 vtxs[8] = vec3[8](
    vec3(-1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3( 1.0,  1.0, -1.0),
    vec3(-1.0, -1.0,  1.0),
    vec3( 1.0, -1.0,  1.0),
    vec3(-1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0)
);

layout(push_constant) uniform PushConsts {
    mat4 view;
    mat4 proj;
} pc;

layout(location = 0) out vec3 out_v_dir;

void main() {
    vec3 position = vtxs[gl_VertexIndex];

    mat4 view_rot = mat4(mat3(pc.view));

    vec4 world_pos = view_rot * vec4(position, 1.0);
    vec4 clip_pos = pc.proj * world_pos;

    gl_Position = vec4(clip_pos.xy, clip_pos.w, clip_pos.w);

    out_v_dir = position;
}

