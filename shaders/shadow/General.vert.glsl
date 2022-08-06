// GLSL Vertex Super Shader

//============================================================================//

#if !defined(OPTION_AnimTexture) ||\
    !defined(OPTION_Bones) ||\
    !defined(OPTION_TexCoord)
  #error
#endif

#if OPTION_AnimTexture && !OPTION_TexCoord
  #error
#endif

//============================================================================//

layout(push_constant, std140)
uniform PushConstants
{
    layout(offset= 0) uint modelMatIndex;
  #if OPTION_AnimTexture
    layout(offset=16) mat2x3 texTransform;
  #endif
}
PC;

layout(set=0, binding=0, std140)
#include "../blocks/Environment.glsl"

layout(set=0, binding=1, std140)
readonly buffer MatricesBlock { mat3x4 MATS[]; };

//============================================================================//

layout(location=0) in vec3 v_Position;

#if OPTION_TexCoord
  layout(location=1) in vec2 v_TexCoord;
#endif

#if OPTION_Bones
  layout(location=5) in ivec4 v_Bones;
  layout(location=6) in vec4 v_Weights;
#endif

#if OPTION_TexCoord
  layout(location=0) out vec2 io_TexCoord;
#endif

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main()
{
    #if OPTION_Bones

      const ivec4 bones = v_Bones + ivec4(1, 1, 1, 1);
      const float weightSum = v_Weights.r + v_Weights.g + v_Weights.b + v_Weights.a;
      const vec4 weights = v_Weights / weightSum;

      vec3 position               = vec4(v_Position, 1.0) * MATS[PC.modelMatIndex + bones.r] * weights.r;
      if (bones.g != 0) position += vec4(v_Position, 1.0) * MATS[PC.modelMatIndex + bones.g] * weights.g;
      if (bones.b != 0) position += vec4(v_Position, 1.0) * MATS[PC.modelMatIndex + bones.b] * weights.b;
      if (bones.a != 0) position += vec4(v_Position, 1.0) * MATS[PC.modelMatIndex + bones.a] * weights.a;

    #else // not OPTION_Bones

      const vec3 position = vec4(v_Position, 1.0) * MATS[PC.modelMatIndex];

    #endif // OPTION_Bones

    #if OPTION_TexCoord && OPTION_AnimTexture
      io_TexCoord = vec3(v_TexCoord, 1.0) * PC.texTransform;
    #elif OPTION_TexCoord
      io_TexCoord = v_TexCoord;
    #endif

    gl_Position = ENV.projViewMatrix * vec4(position, 1.0);
}
