#include "../headers/blocks/Camera.glsl"

layout(set=1, binding=0) uniform sampler2DArray tex_Particles;

#if NUM_SAMPLES == 1
  layout(set=1, binding=1, input_attachment_index=0) uniform subpassInput spi_Depth;
#else
  layout(set=1, binding=1, input_attachment_index=0) uniform subpassInputMS spi_Depth;
#endif

layout(location=0) in GeometryBlock
{
    vec3 texCoord;
    float nearDepth;
    vec3 colour;
    float opacity;
    vec2 fragCoord;
}
IN;

layout(location=0) out vec4 frag_Colour;

void main()
{
    frag_Colour = texture(tex_Particles, IN.texCoord);
    frag_Colour *= vec4(IN.colour, IN.opacity);

  #if NUM_SAMPLES > 1
    float coverage = 0.0, alpha = 0.0;
    gl_SampleMask[0] = 0;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        const float texel = subpassLoad(spi_Depth, i).r;

        // simplified matrix multiply for z only, with constant values folded in
        const float depth = 1.0 / (CB.invProjMat[2][3] * texel + CB.invProjMat[3][3]);

        // todo: particle nearDepth should change away from centre
        const float difference = depth - IN.nearDepth;

        if (difference > 0.0)
        {
            coverage += 1.0;
            alpha += min(difference, 1.0);
            gl_SampleMask[0] |= 1 << i;
        }
    }
    if (coverage != 0.0) alpha /= coverage;
  #else
    const float texel = subpassLoad(spi_Depth).r;
    const float depth = 1.0 / (CB.invProjMat[2][3] * texel + CB.invProjMat[3][3]);
    const float difference = depth - IN.nearDepth;
    const float alpha = clamp(difference, 0.0, 1.0);
  #endif

    frag_Colour.a *= alpha;
}
