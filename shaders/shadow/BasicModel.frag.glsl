// GLSL Fragment Super Shader

//============================================================================//

#if !defined(OPTION_MASK)
  #error
#endif

//============================================================================//

#if OPTION_MASK
  layout(set=1, binding=0) uniform sampler2D tx_Mask;
  layout(location=0) in vec2 io_TexCoord;
#endif

//============================================================================//

void main()
{
    #if OPTION_MASK
      if (texture(tx_Mask, io_TexCoord).r < 0.5) discard;
    #endif
}
