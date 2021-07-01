// Ground Truth Ambient Occlusion
// based on https://github.com/asylum2010/Asylum_Tutorials/blob/master/Media/ShadersGL/gtao.frag

//============================================================================//

// AO_RADIUS:     world space radius of horizon search
// NUM_STEPS:     number of samples to take for each horizon search
// NUM_ROTATIONS: number of rotated iterations to do per pixel
// NUM_OFFSETS:   number of offset iterations to do per pixel
// LOD_BIAS:      tweak the selection of depth mipmap levels

//#define FALLOFF_MODE_SIMPLE

//============================================================================//

#include "../blocks/Camera.glsl"

layout(constant_id=0) const float INVERSE_WIDTH = 0.0;
layout(constant_id=1) const float INVERSE_HEIGHT = 0.0;
layout(constant_id=2) const int MAX_DEPTH_LOD = 0;

#define INVERSE_VIEWPORT vec2(INVERSE_WIDTH, INVERSE_HEIGHT)

layout(set=1, binding=0) uniform sampler2D tx_Normal;
layout(set=1, binding=1) uniform sampler2D tx_Depth;
layout(set=1, binding=2) uniform sampler2D tx_DepthMips;

layout(location=0) out float frag_SSAO;

const float UB_angleOffset = 0.0;
const float UB_spacialOffset = 0.0;

//============================================================================//

vec3 get_view_normal(float depth)
{
    const vec2 texCoord = gl_FragCoord.xy * INVERSE_VIEWPORT;
    const float threshold = 0.001 * depth;

    const vec4 gatherX = textureGather(tx_Normal, texCoord, 0);
    const vec4 gatherY = textureGather(tx_Normal, texCoord, 1);
    const vec4 gatherZ = textureGather(tx_Normal, texCoord, 2);

    const vec4 gatherDepthW = textureGather(tx_Depth, texCoord, 0);
    const vec4 gatherDepth = 1.0 / (gatherDepthW * CAMERA.invProjMat[2][3] + CAMERA.invProjMat[3][3]);

    const vec4 diffs = abs(depth - gatherDepth);

    vec3 worldNormal = vec3(0.0, 0.0, 0.0);
    if (diffs.r < threshold) worldNormal += vec3(gatherX.r, gatherY.r, gatherZ.r);
    if (diffs.g < threshold) worldNormal += vec3(gatherX.g, gatherY.g, gatherZ.g);
    if (diffs.b < threshold) worldNormal += vec3(gatherX.b, gatherY.b, gatherZ.b);
    if (diffs.a < threshold) worldNormal += vec3(gatherX.a, gatherY.a, gatherZ.a);

    const vec3 viewNormal = normalize(mat3(CAMERA.viewMat) * worldNormal);

    return vec3(viewNormal.x, -viewNormal.y, viewNormal.z);
}

//============================================================================//

float get_horizon_sample(vec3 viewPos, vec3 viewDir, float lod, vec2 sampleOffset, float closest)
{
    const vec2 texCoord = (gl_FragCoord.xy + sampleOffset) * INVERSE_VIEWPORT;
    const float depth = textureLod(tx_DepthMips, texCoord, lod).r;

    const vec2 ndc = texCoord * 2.0 - 1.0;
    const vec4 posW = CAMERA.invProjMat * vec4(ndc.x, ndc.y, depth, 1.0);

    const vec3 s = posW.xyz / posW.w;
    const vec3 ws = s - viewPos;

    const float dist2 = dot(ws, ws);
    const float invdist = inversesqrt(dist2);
    const float cosH = dot(ws, viewDir) * invdist;

  #ifdef FALLOFF_MODE_SIMPLE
    // version from the tutorial (no thickness heuristic)
    const float falloff = clamp(dist2 / (2.0 * AO_RADIUS), 0.0, 1.0);
    return max(cosH - (2.0 * falloff), closest);
  #else
    // version from MXAO (thickness heuristic)
    const float falloff = clamp(dist2 / (0.25 * AO_RADIUS * AO_RADIUS), 0.0, 1.0);
    const float foCosH = mix(cosH, closest, falloff);
    return foCosH > closest ? foCosH : mix(foCosH, closest, 0.8);
  #endif
}

//============================================================================//

float compute_visiblity(vec3 viewPos, vec3 viewDir, vec3 viewNorm, float stepSize, float lod, float rotation, float offset)
{
    const vec2 sampleDir = vec2(cos(rotation), sin(rotation));

    // positive and negative horizon cosines
    vec2 horizons = vec2(-1.0, -1.0);
    float sampleDist = 1.0 + stepSize - offset;

    for (int j = 0; j < NUM_STEPS; ++j)
    {
        const vec2 sampleOffset = sampleDir * sampleDist;

        horizons.x = get_horizon_sample(viewPos, viewDir, lod, +sampleOffset, horizons.x);
        horizons.y = get_horizon_sample(viewPos, viewDir, lod, -sampleOffset, horizons.y);

        sampleDist += stepSize;
    }

    horizons = acos(horizons);

    // project normal to the slice plane
    const vec3 bitangent = normalize(cross(vec3(sampleDir, 0.0), viewDir));
    const vec3 tangent = cross(viewDir, bitangent);
    const vec3 projNorm = viewNorm - (bitangent * dot(viewNorm, bitangent));

    const float HALF_PI = 1.570796327;

    // calculate gamma (original code was already optimised)
    const float nnx       = length(projNorm);
    const float invnnx    = 1.0 / (nnx + 0.000001);
    const float cosxi     = dot(projNorm, tangent) * invnnx; // xi = gamma + HALF_PI
    const float gamma     = acos(cosxi) - HALF_PI;
    const float cosgamma  = dot(projNorm, viewDir) * invnnx;
    const float singamma2 = -2.0 * cosxi; // cos(x + HALF_PI) = -sin(x)

    // clamp to normal hemisphere
    horizons.x = gamma + max(-horizons.x - gamma, -HALF_PI);
    horizons.y = gamma + min(horizons.y - gamma, HALF_PI);

    // Riemann integral is additive
    return nnx * 0.25 * (
        (horizons.x * singamma2 + cosgamma - cos(2.0 * horizons.x - gamma)) +
        (horizons.y * singamma2 + cosgamma - cos(2.0 * horizons.y - gamma))
    );
}

//============================================================================//

// transposed, so should be accessed with [y][x]
const vec2 SPATIAL_NOISE[4][4] =
{
    { vec2(0.0625* 0, 0.25*0), vec2(0.0625* 4, 0.25*1), vec2(0.0625* 8, 0.25*2), vec2(0.0625*12, 0.25*3) },
    { vec2(0.0625* 5, 0.25*3), vec2(0.0625* 9, 0.25*0), vec2(0.0625*13, 0.25*1), vec2(0.0625* 1, 0.25*2) },
    { vec2(0.0625*10, 0.25*2), vec2(0.0625*14, 0.25*3), vec2(0.0625* 2, 0.25*0), vec2(0.0625* 6, 0.25*1) },
    { vec2(0.0625*15, 0.25*1), vec2(0.0625* 3, 0.25*2), vec2(0.0625* 7, 0.25*3), vec2(0.0625*11, 0.25*0) }
};

const float TEMPORAL_NOISE[8] = { 0.0, 0.5, 0.25, 0.75, 0.125, 0.375, 0.625, 0.875 };

//============================================================================//

void main()
{
    const float depthW = texelFetch(tx_DepthMips, ivec2(gl_FragCoord), 0).r;
    if (depthW == 1.0) { frag_SSAO = 1.0; return; }

    const vec2 ndcPos = gl_FragCoord.xy * INVERSE_VIEWPORT * 2.0 - 1.0;
    const vec4 viewPosW = CAMERA.invProjMat * vec4(ndcPos.x, ndcPos.y, depthW, 1.0);

    const vec3 viewPos = viewPosW.xyz / viewPosW.w;
    const vec3 viewNorm = get_view_normal(viewPos.z);
    const vec3 viewDir = normalize(-viewPos);

    // convert from view space to pixels (when z is 1.0)
    const float pixelScale = CAMERA.projMat[1][1] / INVERSE_HEIGHT * 0.25;

    // horizon search radius and step size, both in pixels
    const float radius = max((AO_RADIUS * pixelScale) / viewPos.z, NUM_STEPS * 1.415);
    const float stepSize = radius / NUM_STEPS;

    // closer pixels use a higher lod, since there is more space between samples
    const float lod = min(floor(log2(stepSize / NUM_STEPS / 4) + LOD_BIAS), MAX_DEPTH_LOD);

    // per pixel rotation and offset
    const vec2 noises = SPATIAL_NOISE[int(gl_FragCoord.y) % 4][int(gl_FragCoord.x) % 4];

    const float noiseRotation = noises.x / NUM_ROTATIONS;
    const float noiseOffset = noises.y / NUM_OFFSETS;

    frag_SSAO = 0.0;

    for (int r = 0; r < NUM_ROTATIONS; ++r)
    {
        for (int o = 0; o < NUM_OFFSETS; ++o)
        {
            const float combinedRotation = (noiseRotation + TEMPORAL_NOISE[r]) * 3.141592654;
            const float combinedOffset = (noiseOffset + TEMPORAL_NOISE[o]) * stepSize;

            frag_SSAO += compute_visiblity(viewPos, viewDir, viewNorm, stepSize, lod, combinedRotation, combinedOffset);
        }
    }

    frag_SSAO /= float(NUM_ROTATIONS * NUM_OFFSETS);
    frag_SSAO *= frag_SSAO;
}
