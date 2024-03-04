// GLSL Fragment Super Shader

#extension GL_EXT_nonuniform_qualifier : require

//============================================================================//

#if !defined(OPTION_TexMask) ||\
    !defined(OPTION_TexNormal)
  #error
#endif

//============================================================================//

layout(push_constant, std140)
uniform PushConstants
{
  //layout(offset=  0) uint modelMatIndex;
  //layout(offset=  4) uint normalMatIndex;
  //layout(offset=  8) ;
  //layout(offset= 16) mat2x3 texTransform;
    layout(offset= 48) uint albedoTexIndex;
    layout(offset= 52) uint roughnessTexIndex;
  #if OPTION_TexNormal
    layout(offset= 56) uint normalTexIndex;
  #endif
    layout(offset= 60) uint metallicTexIndex;
  #if OPTION_TexMask
    layout(offset= 64) uint maskTexIndex;
  #endif
}
PC;

layout(set=1, binding=0) uniform sampler2D TEXTURES[];

layout(location=0) in vec2 io_TexCoord;
layout(location=1) in vec3 io_Normal;

#if OPTION_TexNormal
  layout(location=2) in vec3 io_Tangent;
  layout(location=3) in vec3 io_Bitangent;
#endif

layout(location=0) out vec4 frag_Albedo_Roughness;
layout(location=1) out vec4 frag_Normal_Metallic;

//============================================================================//

void main()
{
    #if OPTION_TexMask
      if (texture(TEXTURES[PC.maskTexIndex], io_TexCoord).r < 0.5) discard;
    #endif

    frag_Albedo_Roughness.rgb = texture(TEXTURES[PC.albedoTexIndex], io_TexCoord).rgb;

    frag_Albedo_Roughness.a = texture(TEXTURES[PC.roughnessTexIndex], io_TexCoord).r;

    #if OPTION_TexNormal
      const vec3 nm = texture(TEXTURES[PC.normalTexIndex], io_TexCoord).rgb;
      const vec3 T = io_Tangent * nm.x;
      const vec3 B = io_Bitangent * nm.y;
      const vec3 N = io_Normal * nm.z;
      frag_Normal_Metallic.rgb = normalize(T + B + N);
    #else
      frag_Normal_Metallic.rgb = normalize(io_Normal);
    #endif

    if (!gl_FrontFacing)
        frag_Normal_Metallic.rgb = -frag_Normal_Metallic.rgb;

    frag_Normal_Metallic.a = texture(TEXTURES[PC.metallicTexIndex], io_TexCoord).r;
}
