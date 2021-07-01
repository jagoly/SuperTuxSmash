#version 450

#include "../blocks/Camera.glsl"

layout(set=1, binding=0) uniform sampler2DArray tx_Particles;
layout(set=1, binding=1) uniform sampler2D tx_Depth;

layout(location=0) in GeometryBlock
{
    vec3 texCoord;
    float nearDepth;
    vec3 colour;
    float opacity;
}
IN;

layout(location=0) out vec4 frag_Colour;

void main()
{
    frag_Colour = texture(tx_Particles, IN.texCoord).rgba;
    frag_Colour *= vec4(IN.colour, IN.opacity);

    const float depth = texelFetch(tx_Depth, ivec2(gl_FragCoord), 0).r;
    const float linearDepth = 1.0 / (depth * CAMERA.invProjMat[2][3] + CAMERA.invProjMat[3][3]);
    const float difference = linearDepth - IN.nearDepth;

    frag_Colour.a *= clamp(difference, 0.0, 1.0);
}
