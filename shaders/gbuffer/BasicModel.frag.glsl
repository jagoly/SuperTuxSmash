// GLSL Fragment Super Shader

//============================================================================//

#if !defined(OPTION_TEXTURE_MASK) || !defined(OPTION_TEXTURE_NORMAL)
  #error
#endif

//============================================================================//

layout(set=1, binding=0) uniform sampler2D tx_Albedo;
layout(set=1, binding=1) uniform sampler2D tx_Roughness;
layout(set=1, binding=2) uniform sampler2D tx_Metallic;

#if OPTION_TEXTURE_MASK
  layout(set=1, binding=3) uniform sampler2D tx_Mask;
#endif

#if OPTION_TEXTURE_NORMAL
  layout(set=1, binding=4) uniform sampler2D tx_Normal;
#endif

layout(location=0) in vec2 io_TexCoord;
layout(location=1) in vec3 io_Normal;

#if OPTION_TEXTURE_NORMAL
  layout(location=2) in vec3 io_Tangent;
  layout(location=3) in vec3 io_Bitangent;
#endif

layout(location=0) out vec4 frag_Albedo_Roughness;
layout(location=1) out vec4 frag_Normal_Metallic;

//============================================================================//

void main()
{
    #if OPTION_TEXTURE_MASK
      if (texture(tx_Mask, io_TexCoord).r < 0.5) discard;
    #endif

    frag_Albedo_Roughness.rgb = texture(tx_Albedo, io_TexCoord).rgb;

    frag_Albedo_Roughness.a = texture(tx_Roughness, io_TexCoord).r;

    #if OPTION_TEXTURE_NORMAL
      const vec3 nm = texture(tx_Normal, io_TexCoord).rgb;
      const vec3 T = io_Tangent * nm.x;
      const vec3 B = io_Bitangent * nm.y;
      const vec3 N = io_Normal * nm.z;
      frag_Normal_Metallic.rgb = normalize(T + B + N);
    #else
      frag_Normal_Metallic.rgb = normalize(io_Normal);
    #endif

    if (!gl_FrontFacing)
        frag_Normal_Metallic.rgb = -frag_Normal_Metallic.rgb;

    frag_Normal_Metallic.a = texture(tx_Metallic, io_TexCoord).r;
}
