#version 450

//============================================================================//

layout(set=0, binding=0, std140)
#include "../blocks/Camera.glsl"

layout(set=0, binding=1, std140)
#include "../blocks/Environment.glsl"

layout(set=0, binding=2) uniform sampler2DArray tx_Particles;
layout(set=0, binding=3) uniform samplerCube tx_Irradiance;
layout(set=0, binding=4) uniform sampler2D tx_Depth;

layout(location=0) in GeometryBlock
{
    vec3 texCoord;
    float nearDepth;
    vec3 colour;
    float opacity;
}
IN;

layout(location=0) out vec4 frag_Colour;

//============================================================================//

void main()
{
    const vec4 baseColour = texture(tx_Particles, IN.texCoord).rgba;

    const vec2 normXY = IN.texCoord.xy * 2.0 - 1.0;
    const vec3 worldNormFront = mat3(CAMERA.invViewMat) * normalize(vec3(normXY, -1.0));
    const vec3 worldNormSide = mat3(CAMERA.invViewMat) * normalize(vec3(normXY, 0.0));

    // fake scattering by mixing in lighting from the side
    const float weight = (1.0 - length(normXY) / 1.41421356) * baseColour.a;

    const vec3 irradianceFront = texture(tx_Irradiance, worldNormFront * vec3(1,-1,1)).rgb;
    const vec3 irradianceSide = texture(tx_Irradiance, worldNormSide * vec3(1,-1,1)).rgb;
    const vec3 irradiance = mix(irradianceSide, irradianceFront, weight);

    // fake scattering by spreading light over the entire sphere
    const float lightFront = dot(-ENV.lightDirection, worldNormFront) * 0.5 + 0.5;
    const float lightSide = dot(-ENV.lightDirection, worldNormSide) * 0.5 + 0.5;

    // fake some self occlusion by squaring light factor
    const vec3 light = ENV.lightColour * pow(mix(lightSide, lightFront, weight), 2.0) * 0.5;

    const float depth = texelFetch(tx_Depth, ivec2(gl_FragCoord), 0).r;
    const float linearDepth = 1.0 / (depth * CAMERA.invProjMat[2][3] + CAMERA.invProjMat[3][3]);
    const float difference = linearDepth - IN.nearDepth;

    frag_Colour.rgb = baseColour.rgb * IN.colour * (irradiance + light);
    frag_Colour.a = baseColour.a * IN.opacity * clamp(difference, 0.0, 1.0);
}
