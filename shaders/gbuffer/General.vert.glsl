// GLSL Vertex Super Shader

//============================================================================//

#if !defined(OPTION_Bones) ||\
    !defined(OPTION_Tangent)
  #error
#endif

//============================================================================//

layout(push_constant, std140)
uniform PushConstants
{
    layout(offset=  0) uint modelMatIndex;
    layout(offset=  4) uint normalMatIndex;
  //layout(offset=  8) ;
    layout(offset= 16) mat2x3 texTransform;
  //layout(offset= 48) uint albedoTexIndex;
  //layout(offset= 52) uint roughnessTexIndex;
  //layout(offset= 56) uint normalTexIndex;
  //layout(offset= 60) uint metallicTexIndex;
  //layout(offset= 64) uint maskTexIndex;
}
PC;

layout(set=0, binding=0, std140)
#include "../blocks/Camera.glsl"

layout(set=0, binding=1, std140)
readonly buffer MatricesBlock { mat3x4 MATS[]; };

//============================================================================//

layout(location=0) in vec3 v_Position;
layout(location=1) in vec2 v_TexCoord;
layout(location=2) in vec3 v_Normal;

#if OPTION_Tangent
  layout(location=3) in vec4 v_Tangent;
#endif

#if OPTION_Bones
  layout(location=5) in ivec4 v_Bones;
  layout(location=6) in vec4 v_Weights;
#endif

layout(location=0) out vec2 io_TexCoord;
layout(location=1) out vec3 io_Normal;

#if OPTION_Tangent
  layout(location=2) out vec3 io_Tangent;
  layout(location=3) out vec3 io_Bitangent;
#endif

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

      vec3 normal               = v_Normal * mat3(MATS[PC.normalMatIndex + bones.r]) * weights.r;
      if (bones.g != 0) normal += v_Normal * mat3(MATS[PC.normalMatIndex + bones.g]) * weights.g;
      if (bones.b != 0) normal += v_Normal * mat3(MATS[PC.normalMatIndex + bones.b]) * weights.b;
      if (bones.a != 0) normal += v_Normal * mat3(MATS[PC.normalMatIndex + bones.a]) * weights.a;

      #if OPTION_Tangent
        vec3 tangent               = v_Tangent.xyz * mat3(MATS[PC.normalMatIndex + bones.r]) * weights.r;
        if (bones.g != 0) tangent += v_Tangent.xyz * mat3(MATS[PC.normalMatIndex + bones.g]) * weights.g;
        if (bones.b != 0) tangent += v_Tangent.xyz * mat3(MATS[PC.normalMatIndex + bones.b]) * weights.b;
        if (bones.a != 0) tangent += v_Tangent.xyz * mat3(MATS[PC.normalMatIndex + bones.a]) * weights.a;
      #endif

    #else // not OPTION_Bones

      const vec3 position = vec4(v_Position, 1.0) * MATS[PC.modelMatIndex];
      const vec3 normal = v_Normal * mat3(MATS[PC.normalMatIndex]);

      #if OPTION_Tangent
        const vec3 tangent = v_Tangent.xyz * mat3(MATS[PC.normalMatIndex]);
      #endif

    #endif // OPTION_Bones

    io_TexCoord = vec3(v_TexCoord, 1.0) * PC.texTransform;

    io_Normal = normalize(normal);

    #if OPTION_Tangent
      io_Tangent = normalize(tangent);
      io_Bitangent = normalize(cross(normal, tangent) * v_Tangent.w);
    #endif

    gl_Position = CAMERA.projViewMat * vec4(position, 1.0);
}
