#version 450

#include "../headers/blocks/Camera.glsl"

layout(constant_id=0) const float INVERSE_WIDTH = 0.0;
layout(constant_id=1) const float INVERSE_HEIGHT = 0.0;

layout(set=1, binding=0) uniform sampler2D tx_SSAO;
layout(set=1, binding=1) uniform sampler2D tx_DepthHalf;

layout(location=0) out float frag_Blur;

// TODO: investigate taking normals into account for weight
// TODO: investigate using 3x3 and 5x5 noise and filter

//============================================================================//

void gather_ao(vec2 fragCoord, float depth, inout float aoSum, inout float weightSum)
{
    const float MAX_DIFF = 0.04 * depth;

    const vec2 texCoord = fragCoord * vec2(INVERSE_WIDTH, INVERSE_HEIGHT);

    const vec4 gatherDepthW = textureGather(tx_DepthHalf, texCoord, 0);
    const vec4 gatherDepth = 1.0 / (gatherDepthW * CB.invProjMat[2][3] + CB.invProjMat[3][3]);

    const vec4 ao = textureGather(tx_SSAO, texCoord, 0);
    const vec4 weight = max((MAX_DIFF - abs(gatherDepth - depth)) / MAX_DIFF, 0.0);

    aoSum += ao.r * weight.r + ao.g * weight.g + ao.b * weight.b + ao.a * weight.a;
    weightSum += weight.r + weight.g + weight.b + weight.a;
}

//============================================================================//

void main()
{
    const float depthW = texelFetch(tx_DepthHalf, ivec2(gl_FragCoord), 0).r;
    if (depthW == 1.0) { frag_Blur = 1.0; return; }

    const float depth = 1.0 / (depthW * CB.invProjMat[2][3] + CB.invProjMat[3][3]);

    float aoSum = 0.0;
    float weightSum = 0.0;

    gather_ao(gl_FragCoord.xy + vec2(-1.5, -1.5), depth, aoSum, weightSum);
    gather_ao(gl_FragCoord.xy + vec2(-1.5, +0.5), depth, aoSum, weightSum);
    gather_ao(gl_FragCoord.xy + vec2(+0.5, -1.5), depth, aoSum, weightSum);
    gather_ao(gl_FragCoord.xy + vec2(+0.5, +0.5), depth, aoSum, weightSum);

    frag_Blur = aoSum / weightSum;

    //frag_Blur = texelFetch(tx_SSAO, ivec2(gl_FragCoord), 0).r;
}
