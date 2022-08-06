// GLSL Vertex Super Shader

//============================================================================//

#if !defined(OPTION_AnimColour) ||\
    !defined(OPTION_AnimTexture) ||\
    !defined(OPTION_Bones) ||\
    !defined(OPTION_Colour) ||\
    !defined(OPTION_TexCoord)
  #error
#endif

#if OPTION_AnimTexture && !OPTION_TexCoord
  #error
#endif

#if !OPTION_AnimColour && !OPTION_Colour && !OPTION_TexCoord
  #error
#endif

//============================================================================//

layout(push_constant, std140)
uniform PushConstants
{
    layout(offset= 0) uint modelMatIndex;
    layout(offset= 4) uint normalMatIndex;
  #if OPTION_AnimColour
    layout(offset=16) vec4 colour;
  #endif
  #if OPTION_AnimTexture
    layout(offset=32) mat2x3 texTransform;
  #endif
  #if OPTION_Billboard
    layout(offset=64) vec3 bbScale;
  #endif
}
PC;

layout(set=0, binding=0, std140)
#include "../blocks/Camera.glsl"

layout(set=0, binding=1, std140)
readonly buffer MatricesBlock { mat3x4 data[]; } MATS;

//============================================================================//

layout(location=0) in vec3 v_Position;
#if OPTION_TexCoord
  layout(location=1) in vec2 v_TexCoord;
#endif
#if OPTION_Colour
  layout(location=4) in vec4 v_Colour;
#endif
#if OPTION_Bones
  layout(location=5) in ivec4 v_Bones;
  layout(location=6) in vec4 v_Weights;
#endif

layout(location=0) out vec3 io_ViewPos;
#if OPTION_TexCoord
  layout(location=1) out vec2 io_TexCoord;
#endif
#if OPTION_AnimColour || OPTION_Colour
  layout(location=2) out vec4 io_Colour;
#endif

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main()
{
    #if OPTION_Bones

      const float weightSum = v_Weights.r + v_Weights.g + v_Weights.b + v_Weights.a;
      const vec4 weights = v_Weights * (1.0 / weightSum);

      vec3 position                  = vec4(v_Position, 1.0) * MATS.data[PC.modelMatIndex + v_Bones.r] * weights.r;
      if (v_Bones.g != -1) position += vec4(v_Position, 1.0) * MATS.data[PC.modelMatIndex + v_Bones.g] * weights.g;
      if (v_Bones.b != -1) position += vec4(v_Position, 1.0) * MATS.data[PC.modelMatIndex + v_Bones.b] * weights.b;
      if (v_Bones.a != -1) position += vec4(v_Position, 1.0) * MATS.data[PC.modelMatIndex + v_Bones.a] * weights.a;

    #else

      const vec3 position = vec4(v_Position, 1.0) * MATS.data[PC.modelMatIndex];

    #endif

    io_ViewPos = vec3(CAMERA.viewMat * vec4(position, 1.0));

    #if OPTION_TexCoord && OPTION_AnimTexture
      io_TexCoord = vec3(v_TexCoord, 1.0) * PC.texTransform;
    #elif OPTION_TexCoord
      io_TexCoord = v_TexCoord;
    #endif

    #if OPTION_Colour && OPTION_AnimColour
      io_Colour = v_Colour * PC.colour;
    #elif OPTION_Colour
      io_Colour = v_Colour;
    #elif OPTION_AnimColour
      io_Colour = PC.colour;
    #endif

    gl_Position = CAMERA.projMat * vec4(io_ViewPos, 1.0);
}
