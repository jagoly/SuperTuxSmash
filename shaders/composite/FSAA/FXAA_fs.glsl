// GLSL Fragment Shader

#include runtime/Options

//============================================================================//

in vec2 texcrd;

layout(binding=0) uniform sampler2D texScreen;

out vec4 fragColour;

//============================================================================//


/*============================================================================

NVIDIA FXAA 3.11 by TIMOTHY LOTTES 
with edits and trims by James Gangur

------------------------------------------------------------------------------
COPYRIGHT (C) 2010, 2011 NVIDIA CORPORATION. ALL RIGHTS RESERVED.
------------------------------------------------------------------------------
TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL NVIDIA
OR ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR
LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION,
OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE
THIS SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGES.

============================================================================*/


/*============================================================================
FXAA QUALITY - LOW
============================================================================*/

#ifndef OPTION_FSAA_HIGH
#define FXAA_QUALITY_PS 3
#define FXAA_QUALITY_P0 1.5
#define FXAA_QUALITY_P1 3.0
#define FXAA_QUALITY_P2 12.0
#endif


/*============================================================================
FXAA QUALITY - HIGH
============================================================================*/

#ifdef OPTION_FSAA_HIGH
#define FXAA_QUALITY_PS 12
#define FXAA_QUALITY_P0 1.0
#define FXAA_QUALITY_P1 1.0
#define FXAA_QUALITY_P2 1.0
#define FXAA_QUALITY_P3 1.0
#define FXAA_QUALITY_P4 1.0
#define FXAA_QUALITY_P5 1.5
#define FXAA_QUALITY_P6 2.0
#define FXAA_QUALITY_P7 2.0
#define FXAA_QUALITY_P8 2.0
#define FXAA_QUALITY_P9 2.0
#define FXAA_QUALITY_P10 4.0
#define FXAA_QUALITY_P11 8.0
#endif


/*============================================================================
API PORTING (FXAA_GLSL_130)
============================================================================*/

// Requires "#version 130" or better
#define FxaaTexTop(t, p) textureLod(t, p, 0.0)
#define FxaaTexOff(t, p, o, r) textureLodOffset(t, p, 0.0, o)
// use #extension GL_ARB_gpu_shader5 : enable
#define FxaaTexAlpha4(t, p) textureGather(t, p, 3)
#define FxaaTexOffAlpha4(t, p, o) textureGatherOffset(t, p, o, 3)


/*============================================================================
FXAA3 QUALITY SHADER (FXAA_PC)
============================================================================*/

vec4 FxaaPixelShader(

// Use noperspective interpolation here (turn off perspective interpolation).
// {xy} = center of pixel
vec2 pos,

// Input color texture.
// {rgb_} = color in linear or perceptual color space
// {___a} = luma in perceptual color space (not linear)
sampler2D tex,

// This must be from a constant/uniform.
// {x_} = 1.0/screenWidthInPixels
// {_y} = 1.0/screenHeightInPixels
vec2 fxaaQualityRcpFrame,

// Choose the amount of sub-pixel aliasing removal.
// This can effect sharpness.
// 1.00 - upper limit (softer)
// 0.75 - default amount of filtering
// 0.50 - lower limit (sharper, less sub-pixel aliasing removal)
// 0.25 - almost off
// 0.00 - completely off
float fxaaQualitySubpix,

// The minimum amount of local contrast required to apply algorithm.
// 0.333 - too little (faster)
// 0.250 - low quality
// 0.166 - default
// 0.125 - high quality
// 0.063 - overkill (slower)
float fxaaQualityEdgeThreshold,

// Trims the algorithm from processing darks.
// 0.0833 - upper limit (default, the start of visible unfiltered edges)
// 0.0625 - high quality (faster)
// 0.0312 - visible limit (slower)
float fxaaQualityEdgeThresholdMin

) {
/*--------------------------------------------------------------------------*/
vec2 posM;
posM.x = pos.x;
posM.y = pos.y;
vec4 rgbyM = FxaaTexTop(tex, posM);
#define lumaM rgbyM.w
vec4 luma4A = FxaaTexAlpha4(tex, posM);
vec4 luma4B = FxaaTexOffAlpha4(tex, posM, ivec2(-1, -1));
#define lumaE luma4A.z
#define lumaS luma4A.x
#define lumaSE luma4A.y
#define lumaNW luma4B.w
#define lumaN luma4B.z
#define lumaW luma4B.x
/*--------------------------------------------------------------------------*/
float maxSM = max(lumaS, lumaM);
float minSM = min(lumaS, lumaM);
float maxESM = max(lumaE, maxSM);
float minESM = min(lumaE, minSM);
float maxWN = max(lumaN, lumaW);
float minWN = min(lumaN, lumaW);
float rangeMax = max(maxWN, maxESM);
float rangeMin = min(minWN, minESM);
float rangeMaxScaled = rangeMax * fxaaQualityEdgeThreshold;
float range = rangeMax - rangeMin;
float rangeMaxClamped = max(fxaaQualityEdgeThresholdMin, rangeMaxScaled);
bool earlyExit = range < rangeMaxClamped;
/*--------------------------------------------------------------------------*/
if(earlyExit) return rgbyM;
/*--------------------------------------------------------------------------*/
float lumaNE = FxaaTexOff(tex, posM, ivec2(1, -1), fxaaQualityRcpFrame.xy).w;
float lumaSW = FxaaTexOff(tex, posM, ivec2(-1, 1), fxaaQualityRcpFrame.xy).w;
/*--------------------------------------------------------------------------*/
float lumaNS = lumaN + lumaS;
float lumaWE = lumaW + lumaE;
float subpixRcpRange = 1.0/range;
float subpixNSWE = lumaNS + lumaWE;
float edgeHorz1 = (-2.0 * lumaM) + lumaNS;
float edgeVert1 = (-2.0 * lumaM) + lumaWE;
/*--------------------------------------------------------------------------*/
float lumaNESE = lumaNE + lumaSE;
float lumaNWNE = lumaNW + lumaNE;
float edgeHorz2 = (-2.0 * lumaE) + lumaNESE;
float edgeVert2 = (-2.0 * lumaN) + lumaNWNE;
/*--------------------------------------------------------------------------*/
float lumaNWSW = lumaNW + lumaSW;
float lumaSWSE = lumaSW + lumaSE;
float edgeHorz4 = (abs(edgeHorz1) * 2.0) + abs(edgeHorz2);
float edgeVert4 = (abs(edgeVert1) * 2.0) + abs(edgeVert2);
float edgeHorz3 = (-2.0 * lumaW) + lumaNWSW;
float edgeVert3 = (-2.0 * lumaS) + lumaSWSE;
float edgeHorz = abs(edgeHorz3) + edgeHorz4;
float edgeVert = abs(edgeVert3) + edgeVert4;
/*--------------------------------------------------------------------------*/
float subpixNWSWNESE = lumaNWSW + lumaNESE;
float lengthSign = fxaaQualityRcpFrame.x;
bool horzSpan = edgeHorz >= edgeVert;
float subpixA = subpixNSWE * 2.0 + subpixNWSWNESE;
/*--------------------------------------------------------------------------*/
if(!horzSpan) lumaN = lumaW;
if(!horzSpan) lumaS = lumaE;
if(horzSpan) lengthSign = fxaaQualityRcpFrame.y;
float subpixB = (subpixA * (1.0/12.0)) - lumaM;
/*--------------------------------------------------------------------------*/
float gradientN = lumaN - lumaM;
float gradientS = lumaS - lumaM;
float lumaNN = lumaN + lumaM;
float lumaSS = lumaS + lumaM;
bool pairN = abs(gradientN) >= abs(gradientS);
float gradient = max(abs(gradientN), abs(gradientS));
if(pairN) lengthSign = -lengthSign;
float subpixC = clamp(abs(subpixB) * subpixRcpRange, 0.0, 1.0);
/*--------------------------------------------------------------------------*/
vec2 posB;
posB.x = posM.x;
posB.y = posM.y;
vec2 offNP;
offNP.x = (!horzSpan) ? 0.0 : fxaaQualityRcpFrame.x;
offNP.y = ( horzSpan) ? 0.0 : fxaaQualityRcpFrame.y;
if(!horzSpan) posB.x += lengthSign * 0.5;
if( horzSpan) posB.y += lengthSign * 0.5;
/*--------------------------------------------------------------------------*/
vec2 posN;
posN.x = posB.x - offNP.x * FXAA_QUALITY_P0;
posN.y = posB.y - offNP.y * FXAA_QUALITY_P0;
vec2 posP;
posP.x = posB.x + offNP.x * FXAA_QUALITY_P0;
posP.y = posB.y + offNP.y * FXAA_QUALITY_P0;
float subpixD = ((-2.0)*subpixC) + 3.0;
float lumaEndN = FxaaTexTop(tex, posN).w;
float subpixE = subpixC * subpixC;
float lumaEndP = FxaaTexTop(tex, posP).w;
/*--------------------------------------------------------------------------*/
if(!pairN) lumaNN = lumaSS;
float gradientScaled = gradient * 1.0/4.0;
float lumaMM = lumaM - lumaNN * 0.5;
float subpixF = subpixD * subpixE;
bool lumaMLTZero = lumaMM < 0.0;
/*--------------------------------------------------------------------------*/
lumaEndN -= lumaNN * 0.5;
lumaEndP -= lumaNN * 0.5;
bool doneN = abs(lumaEndN) >= gradientScaled;
bool doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P1;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P1;
bool doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P1;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P1;
/*--------------------------------------------------------------------------*/
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P2;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P2;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P2;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P2;
/*--------------------------------------------------------------------------*/
#if (FXAA_QUALITY_PS > 3)
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P3;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P3;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P3;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P3;
/*--------------------------------------------------------------------------*/
#if (FXAA_QUALITY_PS > 4)
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P4;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P4;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P4;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P4;
/*--------------------------------------------------------------------------*/
#if (FXAA_QUALITY_PS > 5)
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P5;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P5;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P5;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P5;
/*--------------------------------------------------------------------------*/
#if (FXAA_QUALITY_PS > 6)
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P6;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P6;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P6;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P6;
/*--------------------------------------------------------------------------*/
#if (FXAA_QUALITY_PS > 7)
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P7;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P7;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P7;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P7;
/*--------------------------------------------------------------------------*/
#if (FXAA_QUALITY_PS > 8)
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P8;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P8;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P8;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P8;
/*--------------------------------------------------------------------------*/
#if (FXAA_QUALITY_PS > 9)
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P9;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P9;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P9;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P9;
/*--------------------------------------------------------------------------*/
#if (FXAA_QUALITY_PS > 10)
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P10;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P10;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P10;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P10;
/*--------------------------------------------------------------------------*/
#if (FXAA_QUALITY_PS > 11)
if(doneNP) {
if(!doneN) lumaEndN = FxaaTexTop(tex, posN.xy).w;
if(!doneP) lumaEndP = FxaaTexTop(tex, posP.xy).w;
if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
doneN = abs(lumaEndN) >= gradientScaled;
doneP = abs(lumaEndP) >= gradientScaled;
if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P11;
if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P11;
doneNP = (!doneN) || (!doneP);
if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P11;
if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P11;
/*--------------------------------------------------------------------------*/
}
#endif
/*--------------------------------------------------------------------------*/
}
#endif
/*--------------------------------------------------------------------------*/
}
#endif
/*--------------------------------------------------------------------------*/
}
#endif
/*--------------------------------------------------------------------------*/
}
#endif
/*--------------------------------------------------------------------------*/
}
#endif
/*--------------------------------------------------------------------------*/
}
#endif
/*--------------------------------------------------------------------------*/
}
#endif
/*--------------------------------------------------------------------------*/
}
#endif
/*--------------------------------------------------------------------------*/
}
/*--------------------------------------------------------------------------*/
float dstN = posM.x - posN.x;
float dstP = posP.x - posM.x;
if(!horzSpan) dstN = posM.y - posN.y;
if(!horzSpan) dstP = posP.y - posM.y;
/*--------------------------------------------------------------------------*/
bool goodSpanN = (lumaEndN < 0.0) != lumaMLTZero;
float spanLength = (dstP + dstN);
bool goodSpanP = (lumaEndP < 0.0) != lumaMLTZero;
float spanLengthRcp = 1.0/spanLength;
/*--------------------------------------------------------------------------*/
bool directionN = dstN < dstP;
float dst = min(dstN, dstP);
bool goodSpan = directionN ? goodSpanN : goodSpanP;
float subpixG = subpixF * subpixF;
float pixelOffset = (dst * (-spanLengthRcp)) + 0.5;
float subpixH = subpixG * fxaaQualitySubpix;
/*--------------------------------------------------------------------------*/
float pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
float pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
if(!horzSpan) posM.x += pixelOffsetSubpix * lengthSign;
if( horzSpan) posM.y += pixelOffsetSubpix * lengthSign;
return vec4(FxaaTexTop(tex, posM).xyz, lumaM);
}
/*==========================================================================*/


//============================================================================//

void main()
{
    const vec2 pixelSize = 1.f / OPTION_WinSizeFull;

    fragColour = FxaaPixelShader( texcrd, texScreen, pixelSize, 0.75f, 0.125f, 0.0625f );
    fragColour.a = 1.f;
}
