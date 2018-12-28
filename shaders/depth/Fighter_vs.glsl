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

    // low precision means we need to normalise this
    const float weightSum = v_Weights.r + v_Weights.g + v_Weights.b + v_Weights.a;
    const vec4 weights = v_Weights * (1.0 / weightSum);

    if (v_Bones.r != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.r] * weights.r;
    if (v_Bones.g != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.g] * weights.g;
    if (v_Bones.b != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.b] * weights.b;
    if (v_Bones.a != -1) position += vec4(v_Position, 1.f) * FB.bones[v_Bones.a] * weights.a;

    //--------------------------------------------------------//

    texcrd = v_TexCoord;

    gl_Position = FB.matrix * vec4(position, 1.f);
}

