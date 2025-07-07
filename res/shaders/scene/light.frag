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
    shadow_uv.z = ndc.z - 0.003; 
    return shadow_uv;
}

void main() {
    vec3 albedo = texture(in_albedo, in_uv).rgb;
    out_color = vec4(albedo, 1.0);
    return;
    vec3 normal = normalize(texture(in_normal, in_uv).xyz * 2.0 - 1.0);
    float depth = texture(in_depth, in_uv).r;

    vec3 world_pos = reconstructWorldPosition(in_uv, depth);

    vec3 shadow_uv = computeShadowUV(world_pos);

    float shadow = 1.0;
    if (all(greaterThanEqual(shadow_uv.xy, vec2(0.0))) &&
        all(lessThanEqual(shadow_uv.xy, vec2(1.0)))) {
        shadow = texture(in_sun_shadow_map, shadow_uv);
    }

    vec3 lighting = albedo * max(globals.sun_color * shadow, vec3(0.05, 0.05, 0.05));
    out_color = vec4(lighting, 1.0);
}
