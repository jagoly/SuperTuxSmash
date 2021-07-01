#version 450

//============================================================================//

layout(constant_id=0) const int SOURCE_SIZE = 0;
layout(constant_id=1) const int OUTPUT_SIZE = 0;
layout(constant_id=2) const int OUTPUT_FACE = 0;

layout(set=0, binding=0) uniform samplerCube tx_Source;

layout(location=0) out vec4 frag_Colour;

#include "Common.glsl"

//============================================================================//

void get_sample(vec3 fragDir, vec2 ndc, int face, inout vec3 valueSum, inout float weightSum)
{
    const vec3 texelDir = get_direction(ndc, face);

    const float weightAngle = max(dot(fragDir, texelDir), 0.0);
    const float weightArea = get_texel_area(ndc);

    const float weight = weightAngle * weightArea;

    valueSum += texture(tx_Source, texelDir).rgb * weight;
    weightSum += weight;
}

//============================================================================//

void main()
{
    const vec2 fragNdc = gl_FragCoord.xy / float(OUTPUT_SIZE) * 2.0 - 1.0;
    const vec3 fragDir = get_direction(fragNdc, OUTPUT_FACE);

    vec3 valueSum = vec3(0.0);
    float weightSum = 0.0;

    // the easiest thing is to just sample every texel of every face
    for (int face = 0; face < 6; ++face)
    {
        for (int offsetX = 1-SOURCE_SIZE; offsetX < SOURCE_SIZE; offsetX += 2)
        {
            for (int offsetY = 1-SOURCE_SIZE; offsetY < SOURCE_SIZE; offsetY += 2)
            {
                const vec2 ndc = vec2(offsetX, offsetY) / float(SOURCE_SIZE);
                get_sample(fragDir, ndc, face, valueSum, weightSum);
            }
        }
    }

    frag_Colour = vec4(valueSum / weightSum, 1.0);
}
