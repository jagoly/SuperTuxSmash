// GLSL Fragment Super Shader

#extension GL_EXT_nonuniform_qualifier : require

//============================================================================//

#if !defined(OPTION_Colour) ||\
    !defined(OPTION_Texture)
  #error
#endif

#if !OPTION_Colour && !OPTION_Texture
  #error
#endif

//============================================================================//

layout(push_constant, std140)
uniform PushConstants
{
  #if OPTION_Texture
    layout(offset= 8) uint colourTexIndex;
  #endif
    layout(offset=12) float blendDepth;
}
PC;

layout(set=0, binding=0, std140)
#include "../blocks/Camera.glsl"

layout(set=0, binding=2) uniform sampler2D tx_Depth;

layout(set=1, binding=0) uniform sampler2D bindlessTextures[];

//============================================================================//

layout(location=0) in vec3 io_ViewPos;
#if OPTION_Texture
  layout(location=1) in vec2 io_TexCoord;
#endif
#if OPTION_Colour
  layout(location=2) in vec4 io_Colour;
#endif

layout(location=0) out vec4 frag_RGBA;

//============================================================================//

void main()
{
    #if OPTION_Texture
      frag_RGBA = texture(bindlessTextures[PC.colourTexIndex], io_TexCoord);
    #else
      frag_RGBA = vec4(1.0, 1.0, 1.0, 1.0);
    #endif

    #if OPTION_Colour
      frag_RGBA *= io_Colour;
    #endif

    const float depth = texelFetch(tx_Depth, ivec2(gl_FragCoord), 0).r;
    const float linearDepth = 1.0 / (depth * CAMERA.invProjMat[2][3] + CAMERA.invProjMat[3][3]);
    const float difference = linearDepth - io_ViewPos.z;

    frag_RGBA.a *= clamp(difference / PC.blendDepth, 0.0, 1.0);
}
