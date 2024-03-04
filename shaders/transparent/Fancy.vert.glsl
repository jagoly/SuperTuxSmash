// GLSL Vertex Super Shader

//============================================================================//

#if !defined(OPTION_Bones) ||\
    !defined(OPTION_Colour) ||\
    !defined(OPTION_TexCoord)
  #error
#endif

#if !OPTION_Colour && !OPTION_TexCoord
  #error
#endif

//============================================================================//

layout(push_constant, std430)
uniform PushConstants
{
    layout(offset=  0) uint modelMatIndex;
  //layout(offset=  4) uint normalMatIndex;
  //layout(offset=  8) uint selection;
  //layout(offset= 12) ;
  #if OPTION_TexCoord
    layout(offset= 16) mat2x3 texTransform;
  #endif
    layout(offset= 48) vec3 diffuse;
    layout(offset= 60) float alpha;
    layout(offset= 64) vec3 emissive;
  //layout(offset= 76) float blendDepth;
  //layout(offset= 80) vec4 colour0;
  //layout(offset= 96) vec4 colour1;
  //layout(offset=112) uint colourTexIndex;
}
PC;

layout(set=0, binding=0, std140)
#include "../blocks/Camera.glsl"

layout(set=0, binding=1, std140)
readonly buffer MatricesBlock { mat3x4 MATS[]; };

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
layout(location=2) out vec4 io_Colour;

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main()
{
    #if OPTION_Bones

      const ivec4 bones = v_Bones + ivec4(1, 1, 1, 1);
      const float weightSum = v_Weights.r + v_Weights.g + v_Weights.b + v_Weights.a;
      const vec4 weights = v_Weights * (1.0 / weightSum);

      vec3 position               = vec4(v_Position, 1.0) * MATS[PC.modelMatIndex + bones.r] * weights.r;
      if (bones.g != 0) position += vec4(v_Position, 1.0) * MATS[PC.modelMatIndex + bones.g] * weights.g;
      if (bones.b != 0) position += vec4(v_Position, 1.0) * MATS[PC.modelMatIndex + bones.b] * weights.b;
      if (bones.a != 0) position += vec4(v_Position, 1.0) * MATS[PC.modelMatIndex + bones.a] * weights.a;

    #else

      const vec3 position = vec4(v_Position, 1.0) * MATS[PC.modelMatIndex];

    #endif

    io_ViewPos = vec3(CAMERA.viewMat * vec4(position, 1.0));

    #if OPTION_TexCoord
      io_TexCoord = vec3(v_TexCoord, 1.0) * PC.texTransform;
    #endif

    // todo: multiply by soft light volume
    const vec3 diffuse = PC.diffuse;

    #if OPTION_Colour
      io_Colour = vec4(diffuse + PC.emissive, PC.alpha) * v_Colour;
    #else
      io_Colour = vec4(diffuse + PC.emissive, PC.alpha);
    #endif

    gl_Position = CAMERA.projMat * vec4(io_ViewPos, 1.0);
}
