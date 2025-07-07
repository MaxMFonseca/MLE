#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D in_albedo;
layout(set = 0, binding = 1) uniform sampler2D in_normal;
layout(set = 0, binding = 2) uniform sampler2D in_material;
layout(set = 0, binding = 3) uniform sampler2D in_depth;

layout(set = 0, binding = 4) uniform GlobalUniforms {
    mat4 view;
    mat4 proj;
    mat4 view_proj;
    mat4 inv_view_proj;
    mat4 sun_light_matrix;
    vec3 sun_direction;
    vec3 sun_color;
    float sun_intensity;
} globals;

layout(set = 0, binding = 5) uniform sampler2DShadow in_sun_shadow_map;

layout(location = 0) out vec4 out_color;

vec3 reconstructWorldPosition(vec2 uv, float depth) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 world = globals.inv_view_proj * clip;
    return world.xyz / world.w;
}

vec3 computeShadowUV(vec3 world_pos) {
    vec4 light_space_pos = globals.sun_light_matrix * vec4(world_pos, 1.0);
    vec3 ndc = light_space_pos.xyz / light_space_pos.w;

    vec3 shadow_uv;
    shadow_uv.xy = ndc.xy * 0.5 + 0.5;
    shadow_uv.z = ndc.z;
    return shadow_uv;
}

float shadowPCF(vec3 shadow_uv, float bias) {
    float shadow = 0.0;

    int kernel_size = 5;
    int half_kernel = kernel_size / 2;

    vec2 texel_size = 1.0 / textureSize(in_sun_shadow_map, 0);
    for (int x = -half_kernel; x <= half_kernel; ++x)
        for (int y = -half_kernel; y <= half_kernel; ++y) {
            vec2 offset = vec2(x, y) * texel_size;
            shadow += texture(in_sun_shadow_map, vec3(shadow_uv.xy + offset, shadow_uv.z - bias));
        }
    return shadow / (kernel_size * kernel_size);
}

void main() {
    vec3 albedo = texture(in_albedo, in_uv).rgb;
    vec3 normal = normalize(texture(in_normal, in_uv).xyz * 2.0 - 1.0);
    float depth = texture(in_depth, in_uv).r;

    vec3 world_pos = reconstructWorldPosition(in_uv, depth);

    vec3 shadow_uv = computeShadowUV(world_pos);

    float shadow = 1.0;
    if (all(greaterThanEqual(shadow_uv.xy, vec2(0.0))) &&
        all(lessThanEqual(shadow_uv.xy, vec2(1.0)))) {
        shadow = shadowPCF(shadow_uv, 0.0017);
    }

    vec3 lighting = albedo * globals.sun_color * max(globals.sun_intensity * shadow, 0.03);
    out_color = vec4(lighting, 1.0);
}
