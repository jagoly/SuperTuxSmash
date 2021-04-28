// GLSL Vertex Super Shader

//============================================================================//

#if !defined(OPTION_SKELLY) ||\
    !defined(OPTION_TANGENTS)
  #error
#endif

//============================================================================//

#include "../headers/blocks/Camera.glsl"

#if OPTION_SKELLY
  #include "../headers/blocks/Skelly.glsl"
#else
  #include "../headers/blocks/Static.glsl"
#endif

//============================================================================//

layout(location=0) in vec3 v_Position;
layout(location=1) in vec2 v_TexCoord;
layout(location=2) in vec3 v_Normal;

#if OPTION_TANGENTS
  layout(location=3) in vec4 v_Tangent;
#endif

#if OPTION_SKELLY
  layout(location=5) in ivec4 v_Bones;
  layout(location=6) in vec4 v_Weights;
#endif

layout(location=0) out vec3 io_ViewPos;
layout(location=1) out vec2 io_TexCoord;
layout(location=2) out vec3 io_Normal;

#if OPTION_TANGENTS
  layout(location=3) out vec3 io_Tangent;
  layout(location=4) out vec3 io_Bitangent;
#endif

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main() 
{
    #if OPTION_SKELLY

      const float weightSum = v_Weights.r + v_Weights.g + v_Weights.b + v_Weights.a;
      const vec4 weights = v_Weights * (1.f / weightSum);

      vec3 position                  = vec4(v_Position, 1.f) * SB.bones[v_Bones.r] * weights.r;
      if (v_Bones.g != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.g] * weights.g;
      if (v_Bones.b != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.b] * weights.b;
      if (v_Bones.a != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.a] * weights.a;

      vec3 normal                  = v_Normal * mat3(SB.bones[v_Bones.r]) * weights.r;
      if (v_Bones.g != -1) normal += v_Normal * mat3(SB.bones[v_Bones.g]) * weights.g;
      if (v_Bones.b != -1) normal += v_Normal * mat3(SB.bones[v_Bones.b]) * weights.b;
      if (v_Bones.a != -1) normal += v_Normal * mat3(SB.bones[v_Bones.a]) * weights.a;

      #if OPTION_TANGENTS
        vec3 tangent                  = v_Tangent.xyz * mat3(SB.bones[v_Bones.r]) * weights.r;
        if (v_Bones.g != -1) tangent += v_Tangent.xyz * mat3(SB.bones[v_Bones.g]) * weights.g;
        if (v_Bones.b != -1) tangent += v_Tangent.xyz * mat3(SB.bones[v_Bones.b]) * weights.b;
        if (v_Bones.a != -1) tangent += v_Tangent.xyz * mat3(SB.bones[v_Bones.a]) * weights.a;
      #endif

    #else // not OPTION_SKELLY

      const vec3 position = v_Position;
      const vec3 normal = v_Normal;

      #if OPTION_TANGENTS
        const vec3 tangent = v_Tangent.xyz;
      #endif

    #endif // OPTION_SKELLY

    io_ViewPos = vec3(CB.invProjMat * SB.matrix * vec4(position, 1.f));

    io_TexCoord = v_TexCoord;

    io_Normal = normalize(SB.normMat * normal);
    
    #if OPTION_TANGENTS
      io_Tangent = normalize(SB.normMat * tangent);
      io_Bitangent = normalize(SB.normMat * cross(normal, tangent) * v_Tangent.w);
    #endif

    gl_Position = SB.matrix * vec4(position, 1.f);
}
