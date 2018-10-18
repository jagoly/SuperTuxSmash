// GLSL Vertex Shader

//============================================================================//

#include headers/blocks/Fighter

layout(std140, binding=2) uniform FIGHTER { FighterBlock FB; };

//============================================================================//

layout(location=0) in vec3 v_Position;
layout(location=1) in vec2 v_TexCoord;
layout(location=5) in ivec4 v_Bones;
layout(location=6) in vec4 v_Weights;

out vec2 texcrd;

//============================================================================//

void main()
{
    vec3 position = vec3(0.f, 0.f, 0.f);

    if (v_Bones.r != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.r] * v_Weights.r;
    if (v_Bones.g != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.g] * v_Weights.g;
    if (v_Bones.b != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.b] * v_Weights.b;
    if (v_Bones.a != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.a] * v_Weights.a;

    //--------------------------------------------------------//

    texcrd = v_TexCoord;

    gl_Position = FB.matrix * vec4(position, 1.f);
}

