// GLSL Vertex Super Shader

//============================================================================//

#include headers/blocks/Camera
#include headers/blocks/Fighter

layout(std140, binding=0) uniform CAMERA { CameraBlock CB; };
layout(std140, binding=2) uniform FIGHTER { FighterBlock FB; };

//============================================================================//

layout(location=0) in vec3 v_Position;
layout(location=1) in vec2 v_TexCoord;
layout(location=2) in vec3 v_Normal;
layout(location=3) in vec4 v_Tangent;
layout(location=5) in ivec4 v_Bones;
layout(location=6) in vec4 v_Weights;

out vec2 texcrd;
out vec3 viewpos;
out vec3 N, T, B;

//============================================================================//

void main() 
{
    vec3 position = vec3(0.f, 0.f, 0.f);
    vec3 normal   = vec3(0.f, 0.f, 0.f);
    vec3 tangent  = vec3(0.f, 0.f, 0.f);

    if (v_Bones.r != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.r] * v_Weights.r;
    if (v_Bones.g != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.g] * v_Weights.g;
    if (v_Bones.b != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.b] * v_Weights.b;
    if (v_Bones.a != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.a] * v_Weights.a;

    if (v_Bones.r != -1) normal += v_Normal * mat3(FB.bones[v_Bones.r]) * v_Weights.r;
    if (v_Bones.g != -1) normal += v_Normal * mat3(FB.bones[v_Bones.g]) * v_Weights.g;
    if (v_Bones.b != -1) normal += v_Normal * mat3(FB.bones[v_Bones.b]) * v_Weights.b;
    if (v_Bones.a != -1) normal += v_Normal * mat3(FB.bones[v_Bones.a]) * v_Weights.a;

    if (v_Bones.r != -1) tangent += v_Tangent.xyz * mat3(FB.bones[v_Bones.r]) * v_Weights.r;
    if (v_Bones.g != -1) tangent += v_Tangent.xyz * mat3(FB.bones[v_Bones.g]) * v_Weights.g;
    if (v_Bones.b != -1) tangent += v_Tangent.xyz * mat3(FB.bones[v_Bones.b]) * v_Weights.b;
    if (v_Bones.a != -1) tangent += v_Tangent.xyz * mat3(FB.bones[v_Bones.a]) * v_Weights.a;

    //--------------------------------------------------------//

    texcrd = v_TexCoord;

    N = normalize(FB.normMat * normal);
    T = normalize(FB.normMat * tangent);
    B = normalize(FB.normMat * cross(normal, tangent) * v_Tangent.w);

    viewpos = vec3(CB.invProjMat * FB.matrix * vec4(position, 1.f));

    gl_Position = FB.matrix * vec4(position, 1.f);
}
