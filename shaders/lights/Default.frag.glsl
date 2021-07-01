// GLSL Fragment Super Shader

//============================================================================//

#if !defined(OPTION_SSAO)
  #error
#endif

#include "../blocks/Camera.glsl"
#include "../blocks/Environment.glsl"

//============================================================================//

layout(set=1, binding=1) uniform samplerCube tx_Irradiance;
layout(set=1, binding=2) uniform samplerCube tx_Radiance;

layout(set=2, binding=0) uniform sampler2D tx_BrdfLut;
layout(set=2, binding=1) uniform sampler2D tx_Albedo_Roughness;
layout(set=2, binding=2) uniform sampler2D tx_Normal_Metallic;
layout(set=2, binding=3) uniform sampler2D tx_Depth;

#if OPTION_SSAO
  layout(set=2, binding=4) uniform sampler2D tx_SSAO;
  layout(set=2, binding=5) uniform sampler2D tx_DepthHalf;
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

// combine baked occlusion with screen space occlusion
float get_occlusion(float depth)
{
  #if OPTION_SSAO
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
    float minDiff = diffs.r, result = ao.r;
    if (diffs.g < minDiff) { minDiff = diffs.g; result = ao.g; }
    if (diffs.b < minDiff) { minDiff = diffs.b; result = ao.b; }
    if (diffs.a < minDiff) { minDiff = diffs.a; result = ao.a; }

    return result;
  #else
    return 1.0;
  #endif
}

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

    return aa / (3.14159265359 * root * root);
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

    // todo: look into calculating SSDO as well for the main light
    const float occlusion = get_occlusion(viewPos.z);

    // indirect lighting
    {
        const vec3 F = fresnel_schlick_roughness(NdotV, F0, roughness);
        const vec2 lookup = texture(tx_BrdfLut, vec2(NdotV, 1.0 - roughness)).rg;

        const float radianceLod = roughness * 7.0; // 128 64 32 16 8 4 2 1

        // todo: flip cube maps during load
        const vec3 radiance = textureLod(tx_Radiance, reflect(-V, N) * vec3(1,-1,1), radianceLod).rgb;
        const vec3 irradiance = texture(tx_Irradiance, N * vec3(1,-1,1)).rgb;

        // temporary workaround until I make a better skybox + add hdr tone mapping options
        const vec3 fakeLight = ENV.ambientColour;

        const vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo * (irradiance + fakeLight);
        const vec3 specular = (F * lookup.x + lookup.y) * (radiance + fakeLight);

        frag_Colour = (diffuse + specular) * occlusion;
    }

    // direct lighting
    {
        const vec3 L = -ENV.lightDirection;
        const vec3 H = normalize(V + L);

        const float NdotL = max(dot(N, L), 0.0);

        // cook torrance brdf
        const vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
        const float G = geometry_smith_schlick_ggx(NdotV, NdotL, roughness);
        const float D = distribution_ggx(N, H, roughness);

        const vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo / 3.14159265359;
        const vec3 specular = F * G * D / max(4.0 * NdotV * NdotL, 0.0001);

        // makes no physical sense, but looks good for now given the lack of shadows
        const float nonsenseOcclusion = mix(occlusion, 1.0, 0.4);

        frag_Colour += (diffuse + specular) * NdotL * ENV.lightColour * nonsenseOcclusion;
    }

    //frag_Colour = vec3(occlusion);
}
