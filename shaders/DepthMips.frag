// Generate a single level of a heirarchical depth buffer
// based on https://miketuritzin.com/post/hierarchical-depth-buffers

//============================================================================//

#version 450

layout(constant_id=0) const int EXTRA_COLUMN = 0;
layout(constant_id=1) const int EXTRA_ROW = 0;

layout(set=0, binding=0) uniform sampler2D tx_Depth;

out float gl_FragDepth;

//============================================================================//

void main()
{
    const ivec2 fragCoord = ivec2(gl_FragCoord) * 2;

    vec4 texels;
    texels.r = texelFetch(tx_Depth, fragCoord + ivec2(0, 0), 0).r;
    texels.g = texelFetch(tx_Depth, fragCoord + ivec2(0, 1), 0).r;
    texels.b = texelFetch(tx_Depth, fragCoord + ivec2(1, 0), 0).r;
    texels.a = texelFetch(tx_Depth, fragCoord + ivec2(1, 1), 0).r;

    float minDepth = min(min(min(texels.r, texels.g), texels.b), texels.a);

    if (EXTRA_COLUMN != 0)
    {
        vec2 extraTexels;
        extraTexels.x = texelFetch(tx_Depth, fragCoord + ivec2(2, 0), 0).r;
        extraTexels.y = texelFetch(tx_Depth, fragCoord + ivec2(2, 1), 0).r;
        minDepth = min(min(minDepth, extraTexels.r), extraTexels.g);
    }

    if (EXTRA_ROW != 0)
    {
        vec2 extraTexels;
        extraTexels.x = texelFetch(tx_Depth, fragCoord + ivec2(0, 2), 0).r;
        extraTexels.y = texelFetch(tx_Depth, fragCoord + ivec2(1, 2), 0).r;
        minDepth = min(min(minDepth, extraTexels.r), extraTexels.g);
    }

    if (EXTRA_COLUMN != 0 && EXTRA_ROW != 0)
    {
        float extraTexel = texelFetch(tx_Depth, fragCoord + ivec2(2, 2), 0).r;
        minDepth = min(minDepth, extraTexel);
    }

    gl_FragDepth = minDepth;
}
