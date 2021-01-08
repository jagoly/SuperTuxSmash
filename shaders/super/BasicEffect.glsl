// GLSL Combined Super Shader

//============================================================================//

layout(location=0, binding=0) uniform sampler2D tx_Colour;

//----------------------------------------------------------------------------//

#include headers/blocks/Camera
#include headers/blocks/Effect

#line 12

//============================================================================//

#if defined(SHADER_STAGE_VERTEX)

//----------------------------------------------------------------------------//

layout(location=0) in vec3 v_Position;
layout(location=1) in vec2 v_TexCoord;
layout(location=5) in ivec4 v_Bones;
layout(location=6) in vec4 v_Weights;

layout(location=0) out vec3 io_ViewPos;
layout(location=1) out vec2 io_TexCoord;
layout(location=2) out vec4 io_Params;

//----------------------------------------------------------------------------//

void main() 
{
    const float weightSum = v_Weights.r + v_Weights.g + v_Weights.b + v_Weights.a;
    const vec4 weights = v_Weights * (1.f / weightSum);

    vec3 position                  = vec4(v_Position, 1.f) * EB.bones[v_Bones.r] * weights.r;
    if (v_Bones.g != -1) position += vec4(v_Position, 1.f) * EB.bones[v_Bones.g] * weights.g;
    if (v_Bones.b != -1) position += vec4(v_Position, 1.f) * EB.bones[v_Bones.b] * weights.b;
    if (v_Bones.a != -1) position += vec4(v_Position, 1.f) * EB.bones[v_Bones.a] * weights.a;

    io_ViewPos = vec3(CB.invProjMat * EB.matrix * vec4(position, 1.f));
    io_TexCoord = v_TexCoord;
    
    // note: could be blended from multiple tracks, for now just use the first
    io_Params = EB.params[v_Bones.r];

    gl_Position = EB.matrix * vec4(position, 1.f);
}

#endif // SHADER_STAGE_VERTEX

//============================================================================//

#if defined(SHADER_STAGE_FRAGMENT)

//----------------------------------------------------------------------------//

layout(location=0) in vec3 io_ViewPos;
layout(location=1) in vec2 io_TexCoord;
layout(location=2) in vec4 io_Params;

layout(location=0) out vec4 frag_RGBA;

//----------------------------------------------------------------------------//

void main()
{
    frag_RGBA = texture(tx_Colour, io_TexCoord) * io_Params;
    //frag_RGBA.gba = vec3(1,1,1);
}

#endif // SHADER_STAGE_FRAGMENT
