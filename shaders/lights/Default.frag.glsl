// GLSL Fragment Super Shader

//============================================================================//

#if !defined(OPTION_SHADOW) || !defined(OPTION_SSAO)
  #error
#endif

//============================================================================//

layout(constant_id=0) const float MAX_RADIANCE_LOD = 0.0;
layout(constant_id=1) const float SHADOW_PIXEL_SIZE = 0.0;
layout(constant_id=2) const int SHADOW_PCF_QUALITY = 0;

layout(set=0, binding=0, std140)
#include "../blocks/Camera.glsl"

layout(set=0, binding=1, std140)
#include "../blocks/Environment.glsl"

layout(set=0, binding=2) uniform sampler2D tx_BrdfLut;
layout(set=0, binding=3) uniform samplerCube tx_Irradiance;
layout(set=0, binding=4) uniform samplerCube tx_Radiance;
layout(set=0, binding=5) uniform sampler2D tx_Albedo_Roughness;
layout(set=0, binding=6) uniform sampler2D tx_Normal_Metallic;
layout(set=0, binding=7) uniform sampler2D tx_Depth;

#if OPTION_SHADOW
  layout(set=0, binding=8) uniform sampler2DShadow tx_Shadow;
#endif

#if OPTION_SSAO
  layout(set=0, binding=9) uniform sampler2D tx_SSAO;
  layout(set=0, binding=10) uniform sampler2D tx_DepthHalf;
#endif

layout(location=0) in vec2 io_TexCoord;

layout(location=0) out vec3 frag_Colour;

//============================================================================//

vec3 get_view_position()
{
    const float depth = texture(tx_Depth, io_TexCoord).r;

    const vec2 ndcPos = io_TexCoord * 2.0 - 1.0;
    const vec4 viewPosW = CAMERA.invProjMat * vec4(ndcPos.x, -ndcPos.y, depth, 1.0);

    return viewPosW.xyz / viewPosW.w;
}

//============================================================================//

#if OPTION_SHADOW

float get_shadow(vec3 worldPos, vec3 worldNorm, float NdotL)
{
    // TODO: use non-midpoint shadow maps with a bias for faster low quality shadows
    // TODO: add contact hardening for high quality shadows (PCSS)

    const vec3 shadowPos = vec3(ENV.projViewMatrix * vec4(worldPos + worldNorm * NdotL * 0.01, 1.0));
    const vec2 shadowCoord = shadowPos.xy * vec2(0.5, -0.5) + 0.5;
    const float shadowDepth = shadowPos.z;// - 0.005;

    vec2 begin, end; float sampleCount;
    if (SHADOW_PCF_QUALITY == 1) { begin = vec2(-0.5); end = vec2(+0.5); sampleCount =   4.0; }
    if (SHADOW_PCF_QUALITY == 2) { begin = vec2(-2.5); end = vec2(+2.5); sampleCount =  36.0; }
    if (SHADOW_PCF_QUALITY == 3) { begin = vec2(-4.5); end = vec2(+4.5); sampleCount = 100.0; }
    //if (SHADOW_PCF_QUALITY == 1) { begin = vec2(-1.0); end = vec2(+1.0); sampleCount =   9.0; }
    //if (SHADOW_PCF_QUALITY == 2) { begin = vec2(-3.0); end = vec2(+3.0); sampleCount =  49.0; }
    //if (SHADOW_PCF_QUALITY == 3) { begin = vec2(-5.0); end = vec2(+5.0); sampleCount = 121.0; }

    float result = 0.0;

    for (float y = begin.y; y <= end.y; y += 1.0)
        for (float x = begin.x; x <= end.x; x += 1.0)
            result += texture(tx_Shadow, vec3(shadowCoord + vec2(x, y) * SHADOW_PIXEL_SIZE, shadowDepth));

    return result / sampleCount;
}

#endif

//============================================================================//

#if OPTION_SSAO

float get_ssao(float depth)
{
    const float MAX_DIFF = 0.04 * depth;

    const vec4 depthGatherW = textureGather(tx_DepthHalf, io_TexCoord);
    const vec4 depthGather = 1.0 / (depthGatherW * CAMERA.invProjMat[2][3] + CAMERA.invProjMat[3][3]);

    const vec4 diffs = abs(depthGather - depth);
    const vec4 ao = textureGather(tx_SSAO, io_TexCoord);

    // high quality depth-aware bilateral filter
    if (min(min(min(diffs.r, diffs.g), diffs.b), diffs.a) < MAX_DIFF)
    {
        const vec2 weightXY = 0.75 - 0.5 * (ivec2(gl_FragCoord) & 1);

        vec4 weight = vec4 (
            (1.0 - weightXY.x) * weightXY.y,         // bottom left
            weightXY.x         * weightXY.y,         // bottom right
            weightXY.x         * (1.0 - weightXY.y), // top right
            (1.0 - weightXY.x) * (1.0 - weightXY.y)  // top left
        );

        weight *= max(MAX_DIFF - diffs, 0.0) / MAX_DIFF;

        const float aoSum = ao.r * weight.r + ao.g * weight.g + ao.b * weight.b + ao.a * weight.a;
        const float weightSum = weight.r + weight.g + weight.b + weight.a;

        return aoSum / weightSum;
    }

    // no pixels are within the threshold, so just use the closest one
    float minDiff = diffs.r; float result = ao.r;
    if (diffs.g < minDiff) { minDiff = diffs.g; result = ao.g; }
    if (diffs.b < minDiff) { minDiff = diffs.b; result = ao.b; }
    if (diffs.a < minDiff) { minDiff = diffs.a; result = ao.a; }

    return result;
}

#endif

//============================================================================//

vec3 fresnel_schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float geometry_smith_schlick_ggx(float NdotV, float NdotL, float roughness)
{
    const float k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;

    const float ggxV = NdotV / (NdotV * (1.0 - k) + k);
    const float ggxL = NdotL / (NdotL * (1.0 - k) + k);

    return ggxV * ggxL;
}

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    const float NdotH = max(dot(N, H), 0.0);
    const float aa = roughness * roughness * roughness * roughness;
    const float root = 1.0 + NdotH * NdotH * (aa - 1.0);

    return aa / (3.141592654 * root * root);
}

//============================================================================//

void main()
{
    const vec3 albedo = texture(tx_Albedo_Roughness, io_TexCoord).rgb;
    const float roughness = texture(tx_Albedo_Roughness, io_TexCoord).a;
    const vec3 normal = texture(tx_Normal_Metallic, io_TexCoord).rgb;
    const float metallic = texture(tx_Normal_Metallic, io_TexCoord).a;

    const vec3 viewPos = get_view_position();
    const vec3 worldPos = vec3(CAMERA.invViewMat * vec4(viewPos, 1.0));

    const vec3 N = normalize(normal);
    const vec3 V = normalize(CAMERA.invViewMat[3].xyz - worldPos);

    const vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // non-zero to prevent black spots on normals pointing away from camera
    const float NdotV = max(dot(N, V), 0.0001);

    // indirect lighting
    {
        const vec3 F = fresnel_schlick_roughness(NdotV, F0, roughness);
        const vec2 lookup = texture(tx_BrdfLut, vec2(NdotV, 1.0 - roughness)).rg;

        const float radianceLod = roughness * MAX_RADIANCE_LOD;

        // todo: flip cube maps during load
        const vec3 radiance = textureLod(tx_Radiance, reflect(-V, N) * vec3(1,-1,1), radianceLod).rgb;
        const vec3 irradiance = texture(tx_Irradiance, N * vec3(1,-1,1)).rgb;

        const vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo * irradiance;
        const vec3 specular = (F * lookup.x + lookup.y) * radiance;

        #if OPTION_SSAO
          //const float occlusion = min(texture(tx_Occlusion, io_TexCoord).r, get_ssao(viewPos.z));
          const float occlusion = get_ssao(viewPos.z);
        #else
          //const float occlusion = texture(tx_Occlusion, io_TexCoord).r;
          const float occlusion = 1.0;
        #endif

        frag_Colour = (diffuse + specular) * occlusion;
    }

    // direct lighting
    {
        const vec3 L = -ENV.lightDirection;
        const float NdotL = dot(N, L);

        if (NdotL > 0.0)
        {
            const vec3 H = normalize(V + L);

            // cook torrance brdf
            const vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
            const float G = geometry_smith_schlick_ggx(NdotV, NdotL, roughness);
            const float D = distribution_ggx(N, H, roughness);

            const vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo / 3.141592654;
            const vec3 specular = F * G * D / max(4.0 * NdotV * NdotL, 0.0001);

            #if OPTION_SHADOW
              const float shadow = get_shadow(worldPos, N, 1.0 -NdotL);
            #else
              const float shadow = 1.0;
            #endif

            frag_Colour += (diffuse + specular) * NdotL * ENV.lightColour * shadow;
        }
    }
}
