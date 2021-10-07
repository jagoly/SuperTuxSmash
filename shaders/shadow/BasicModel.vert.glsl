// GLSL Vertex Super Shader

//============================================================================//

layout(std140, set=0, binding=0)
#include "../blocks/Environment.glsl"

#if OPTION_SKELLY
  layout(std140, set=2, binding=0)
  #include "../blocks/Skelly.glsl"
#else
  layout(std140, set=2, binding=0)
  #include "../blocks/Static.glsl"
#endif

//============================================================================//

layout(location=0) in vec3 v_Position;

#if OPTION_MASK
  layout(location=1) in vec2 v_TexCoord;
#endif

#if OPTION_SKELLY
  layout(location=5) in ivec4 v_Bones;
  layout(location=6) in vec4 v_Weights;
#endif

#if OPTION_MASK
  layout(location=0) out vec2 io_TexCoord;
#endif

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main()
{
    #if OPTION_SKELLY

      const float weightSum = v_Weights.r + v_Weights.g + v_Weights.b + v_Weights.a;
      const vec4 weights = v_Weights / weightSum;

      vec3 position                  = vec4(v_Position, 1.0) * MODEL.bones[v_Bones.r] * weights.r;
      if (v_Bones.g != -1) position += vec4(v_Position, 1.0) * MODEL.bones[v_Bones.g] * weights.g;
      if (v_Bones.b != -1) position += vec4(v_Position, 1.0) * MODEL.bones[v_Bones.b] * weights.b;
      if (v_Bones.a != -1) position += vec4(v_Position, 1.0) * MODEL.bones[v_Bones.a] * weights.a;

    #else // not OPTION_SKELLY

      const vec3 position = v_Position;

    #endif // OPTION_SKELLY

    const vec4 worldPos = MODEL.matrix * vec4(position, 1.0);

    #if OPTION_MASK
      io_TexCoord = v_TexCoord;
    #endif

    gl_Position = ENV.projViewMatrix * worldPos;
}
