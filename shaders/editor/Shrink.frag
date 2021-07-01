#version 450

//============================================================================//

layout(constant_id=0) const int SOURCE_SIZE = 0;
layout(constant_id=1) const int OUTPUT_SIZE = 0;

layout(set=0, binding=0) uniform sampler2D tx_Source;

layout(location=0) out vec4 frag_Colour;

#include "Common.glsl"

//============================================================================//

void main()
{
    const int SHRINK_FACTOR = SOURCE_SIZE / OUTPUT_SIZE;

    const ivec2 baseCoord = ivec2(gl_FragCoord.xy * float(SHRINK_FACTOR));

    const ivec2 minCoord = baseCoord - (SHRINK_FACTOR / 2) + 1;
    const ivec2 maxCoord = baseCoord + (SHRINK_FACTOR / 2);

    vec3 valueSum = vec3(0.0);
    float weightSum = 0.0;

    for (int coordX = minCoord.x; coordX <= maxCoord.x; ++coordX)
    {
        for (int coordY = minCoord.y; coordY <= maxCoord.y; ++coordY)
        {
            const ivec2 coord = ivec2(coordX, coordY);
            const vec2 ndc = (vec2(coord) + 0.5) / float(SOURCE_SIZE) * 2.0 - 1.0;

            const float weight = get_texel_area(ndc);

            valueSum += texelFetch(tx_Source, coord, 0).rgb * weight;
            weightSum += weight;
        }
    }

   frag_Colour = vec4(valueSum / weightSum, 1.0);
}
