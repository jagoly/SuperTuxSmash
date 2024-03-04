// GLSL Fragment Super Shader

#extension GL_EXT_nonuniform_qualifier : require

//============================================================================//

#if !defined(OPTION_Texture)
  #error
#endif

//============================================================================//

layout(push_constant, std430)
uniform PushConstants
{
  //layout(offset=  0) uint modelMatIndex;
  //layout(offset=  4) uint normalMatIndex;
    layout(offset=  8) uint selection;
  //layout(offset= 12) ;
  //layout(offset= 16) mat2x3 texTransform;
  //layout(offset= 48) vec3 diffuse;
  //layout(offset= 60) float alpha;
  //layout(offset= 64) vec3 emissive;
    layout(offset= 76) float blendDepth;
    layout(offset= 80) vec4 colour0;
    layout(offset= 96) vec4 colour1;
  #if OPTION_Texture
    layout(offset=112) uint colourTexIndex;
  #endif
}
PC;

layout(set=0, binding=0, std140)
#include "../blocks/Camera.glsl"

layout(set=0, binding=2) uniform sampler2D tx_Depth;

layout(set=1, binding=0) uniform sampler2D TEXTURES[];

//============================================================================//

layout(location=0) in vec3 io_ViewPos;
#if OPTION_Texture
  layout(location=1) in vec2 io_TexCoord;
#endif
layout(location=2) in vec4 io_Colour;

layout(location=0) out vec4 frag_RGBA;

//============================================================================//

vec4 get_selection(uint selection)
{
    if (selection == 0x0) return vec4(0.0, 0.0, 0.0, 0.0);
    if (selection == 0x1) return vec4(1.0, 1.0, 1.0, 1.0);
    #if OPTION_Texture
    if (selection == 0x2) return texture(TEXTURES[PC.colourTexIndex], io_TexCoord);
    #endif
    if (selection == 0x3) return PC.colour0;
    if (selection == 0x4) return PC.colour1;
    //if (selection == 0x5) return PC.colour0 * io_Colour;
    //if (selection == 0x6) return PC.colour1 * io_Colour;
    /*if (selection == 0x7)*/ return io_Colour;
}

//============================================================================//

void main()
{
    const vec3 colourA = get_selection(bitfieldExtract(PC.selection, 28, 4)).rgb;
    const vec3 colourB = get_selection(bitfieldExtract(PC.selection, 24, 4)).rgb;
    const vec3 colourC = get_selection(bitfieldExtract(PC.selection, 20, 4)).rgb;
    const vec3 colourD = get_selection(bitfieldExtract(PC.selection, 16, 4)).rgb;

    frag_RGBA.rgb = colourD + mix(colourA, colourB, colourC);

    const float alphaA = get_selection(bitfieldExtract(PC.selection, 12, 4)).a;
    const float alphaB = get_selection(bitfieldExtract(PC.selection,  8, 4)).a;
    const float alphaC = get_selection(bitfieldExtract(PC.selection,  4, 4)).a;
    const float alphaD = get_selection(bitfieldExtract(PC.selection,  0, 4)).a;

    frag_RGBA.a = alphaD + mix(alphaA, alphaB, alphaC);

    const float depth = texelFetch(tx_Depth, ivec2(gl_FragCoord), 0).r;
    const float linearDepth = 1.0 / (depth * CAMERA.invProjMat[2][3] + CAMERA.invProjMat[3][3]);
    const float difference = linearDepth - io_ViewPos.z;

    frag_RGBA.a *= clamp(difference / PC.blendDepth, 0.0, 1.0);
}
