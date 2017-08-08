// GLSL Vertex Shader

//============================================================================//

#include headers/blocks/Skeleton

layout(std140, binding=2) uniform SKELETON { SkeletonBlock SB; };

//============================================================================//

layout(location=0) in vec3 v_Position;
layout(location=1) in vec2 v_TexCoord;
layout(location=5) in ivec4 v_Bones;
layout(location=6) in vec4 v_Weights;

layout(location=0) uniform mat4 u_Matrix;

out vec2 texcrd;

//============================================================================//

void main()
{
    vec3 position = vec3(0.f, 0.f, 0.f);

    if (v_Bones.r != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.r] * v_Weights.r;
    if (v_Bones.g != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.g] * v_Weights.g;
    if (v_Bones.b != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.b] * v_Weights.b;
    if (v_Bones.a != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.a] * v_Weights.a;

    //--------------------------------------------------------//

    texcrd = v_TexCoord;

    gl_Position = u_Matrix * vec4(position, 1.f);
}

