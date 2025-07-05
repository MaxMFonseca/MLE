#version 450

layout(push_constant) uniform PushConsts {
    mat4 view;
    mat4 proj;
} pc;

layout(location = 0) out vec3 v_dir;

// The cube vertex positions baked into the shader
vec3 getVertex(uint index) {
    const vec3 cube[8] = vec3[8](
        vec3(-1.0, -1.0, -1.0), // 0
        vec3( 1.0, -1.0, -1.0), // 1
        vec3(-1.0,  1.0, -1.0), // 2
        vec3( 1.0,  1.0, -1.0), // 3
        vec3(-1.0, -1.0,  1.0), // 4
        vec3( 1.0, -1.0,  1.0), // 5
        vec3(-1.0,  1.0,  1.0), // 6
        vec3( 1.0,  1.0,  1.0)  // 7
    );
    return cube[index];
}

void main() {
    vec3 position = getVertex(gl_VertexIndex);

    // Strip translation from view matrix
    mat4 view_rot = mat4(mat3(pc.view));

    vec4 world_pos = view_rot * vec4(position, 1.0);
    vec4 clip_pos = pc.proj * world_pos;

    // Depth = 1.0 so skybox stays at far plane
    gl_Position = vec4(clip_pos.xy, clip_pos.w, clip_pos.w);

    v_dir = position;
}

