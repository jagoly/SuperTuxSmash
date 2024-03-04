// GLSL Fragment Super Shader

#extension GL_EXT_nonuniform_qualifier : require

//============================================================================//

#if !defined(OPTION_TexMask)
  #error
#endif

//============================================================================//

#if OPTION_TexMask
layout(push_constant, std140)
uniform PushConstants
{
  //layout(offset=  0) uint modelMatIndex;
  //layout(offset=  4) ;
  //layout(offset= 16) mat2x3 texTransform;
    layout(offset= 48) uint maskTexIndex;
}
PC;
#endif

layout(set=1, binding=0) uniform sampler2D TEXTURES[];

#if OPTION_TexMask
  layout(location=0) in vec2 io_TexCoord;
#endif

//============================================================================//

void main()
{
    #if OPTION_TexMask
      if (texture(TEXTURES[PC.maskTexIndex], io_TexCoord).r < 0.5) discard;
    #endif
}
