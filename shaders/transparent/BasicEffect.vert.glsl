// GLSL Vertex Super Shader

//============================================================================//

// TODO: options

//============================================================================//

#include "../blocks/Camera.glsl"
#include "../blocks/Effect.glsl"

//============================================================================//

layout(location=0) in vec3 v_Position;
layout(location=1) in vec2 v_TexCoord;
layout(location=5) in ivec4 v_Bones;
layout(location=6) in vec4 v_Weights;

layout(location=0) out vec3 io_ViewPos;
layout(location=1) out vec2 io_TexCoord;
layout(location=2) out vec4 io_Params;

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main() 
{
    const float weightSum = v_Weights.r + v_Weights.g + v_Weights.b + v_Weights.a;
    const vec4 weights = v_Weights * (1.0 / weightSum);

    vec3 position                  = vec4(v_Position, 1.0) * EFFECT.bones[v_Bones.r] * weights.r;
    if (v_Bones.g != -1) position += vec4(v_Position, 1.0) * EFFECT.bones[v_Bones.g] * weights.g;
    if (v_Bones.b != -1) position += vec4(v_Position, 1.0) * EFFECT.bones[v_Bones.b] * weights.b;
    if (v_Bones.a != -1) position += vec4(v_Position, 1.0) * EFFECT.bones[v_Bones.a] * weights.a;

    io_ViewPos = vec3(CAMERA.invProjMat * EFFECT.matrix * vec4(position, 1.0));
    io_TexCoord = v_TexCoord;
    
    // note: could be blended from multiple tracks, for now just use the first
    io_Params = EFFECT.params[v_Bones.r];

    gl_Position = EFFECT.matrix * vec4(position, 1.0);
}
