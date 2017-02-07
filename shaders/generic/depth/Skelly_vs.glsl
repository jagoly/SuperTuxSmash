// GLSL Vertex Shader

//============================================================================//

#include headers/blocks/Skeleton

layout(std140, binding=2) uniform SKELETONBLOCK { SkeletonBlock SB; };

//============================================================================//

layout(location=0) in vec3 V_pos;
layout(location=1) in vec2 V_tcrd;
layout(location=5) in ivec4 V_bones;
layout(location=6) in vec4 V_weights;

uniform mat4 u_final_mat;

out vec2 texcrd;

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main() 
{
    // ensure that bone weights are normalized
    const vec4 weights = V_weights / dot(V_weights, vec4(1.f));

    vec3 a_pos = vec3(0.f, 0.f, 0.f);

    if (V_bones.r >= 0) a_pos += vec4(V_pos, 1.f) * SB.bones[V_bones.r] * weights.r;
    if (V_bones.g >= 0) a_pos += vec4(V_pos, 1.f) * SB.bones[V_bones.g] * weights.g;
    if (V_bones.b >= 0) a_pos += vec4(V_pos, 1.f) * SB.bones[V_bones.b] * weights.b;
    if (V_bones.a >= 0) a_pos += vec4(V_pos, 1.f) * SB.bones[V_bones.a] * weights.a;

    texcrd = V_tcrd;

    gl_Position = u_final_mat * vec4(a_pos, 1.f);
}
