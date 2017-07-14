// GLSL Vertex Super Shader

//============================================================================//

#include headers/blocks/Camera
#include headers/blocks/Skeleton

layout(std140, binding=0) uniform CAMERABLOCK { CameraBlock CB; };
layout(std140, binding=2) uniform SKELETONBLOCK { SkeletonBlock SB; };

//============================================================================//

layout(location=0) in vec3 V_pos;
layout(location=1) in vec2 V_tcrd;
layout(location=2) in vec3 V_norm;
layout(location=3) in vec4 V_tan;
layout(location=5) in ivec4 V_bones;
layout(location=6) in vec4 V_weights;

layout(location=0) uniform mat4 u_final_mat;
layout(location=1) uniform mat3 u_normal_mat;

out vec2 texcrd;
out vec3 viewpos;
out vec3 N, T, B;

//============================================================================//

void main() 
{
    // ensure that bone weights are normalized
    const vec4 weights = V_weights / dot(V_weights, vec4(1.f));

    vec3 a_pos = vec3(0.f, 0.f, 0.f);
    vec3 a_norm = vec3(0.f, 0.f, 0.f);
    vec3 a_tan = vec3(0.f, 0.f, 0.f);

    if (V_bones.r >= 0) a_pos += vec4(V_pos, 1.f) * SB.bones[V_bones.r] * weights.r;
    if (V_bones.g >= 0) a_pos += vec4(V_pos, 1.f) * SB.bones[V_bones.g] * weights.g;
    if (V_bones.b >= 0) a_pos += vec4(V_pos, 1.f) * SB.bones[V_bones.b] * weights.b;
    if (V_bones.a >= 0) a_pos += vec4(V_pos, 1.f) * SB.bones[V_bones.a] * weights.a;

    if (V_bones.r >= 0) a_norm += V_norm * mat3(SB.bones[V_bones.r]) * weights.r;
    if (V_bones.g >= 0) a_norm += V_norm * mat3(SB.bones[V_bones.g]) * weights.g;
    if (V_bones.b >= 0) a_norm += V_norm * mat3(SB.bones[V_bones.b]) * weights.b;
    if (V_bones.a >= 0) a_norm += V_norm * mat3(SB.bones[V_bones.a]) * weights.a;

    if (V_bones.r >= 0) a_tan += V_tan.xyz * mat3(SB.bones[V_bones.r]) * weights.r;
    if (V_bones.g >= 0) a_tan += V_tan.xyz * mat3(SB.bones[V_bones.g]) * weights.g;
    if (V_bones.b >= 0) a_tan += V_tan.xyz * mat3(SB.bones[V_bones.b]) * weights.b;
    if (V_bones.a >= 0) a_tan += V_tan.xyz * mat3(SB.bones[V_bones.a]) * weights.a;

    texcrd = V_tcrd;

    N = normalize(u_normal_mat * a_norm);
    T = normalize(u_normal_mat * a_tan);
    B = normalize(u_normal_mat * cross(a_norm, a_tan) * V_tan.w);

    viewpos = vec3(CB.proj_inv * u_final_mat * vec4(a_pos, 1.f));
    gl_Position = u_final_mat * vec4(a_pos, 1.f);
}
