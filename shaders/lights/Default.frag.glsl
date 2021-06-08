// GLSL Fragment Super Shader

//============================================================================//

//#if !defined(OPTION_SSAO)
//  #error
//#endif

#include "../headers/blocks/Camera.glsl"
#include "../headers/blocks/Light.glsl"

//============================================================================//

layout(set=1, binding=1) uniform sampler2D tx_BrdfLut;
layout(set=1, binding=2) uniform samplerCube tx_Irradiance;
layout(set=1, binding=3) uniform samplerCube tx_Radiance;

layout(set=1, binding=4) uniform sampler2D tx_Albedo_Roughness;
layout(set=1, binding=5) uniform sampler2D tx_Normal_Metallic;
layout(set=1, binding=6) uniform sampler2D tx_Depth;

//#if OPTION_SSAO
//  layout(set=0, binding=7) uniform sampler2D tx_HalfDepth;
//  layout(set=0, binding=8) uniform sampler2D tx_SSAO;
//#endif

layout(location=0) in vec2 io_TexCoord;

layout(location=0) out vec3 frag_Colour;

//============================================================================//

vec3 get_world_position()
{
    const float depth = texture(tx_Depth, io_TexCoord).r;

    const vec4 clipPos = vec4(vec2(io_TexCoord.x, 1.0 - io_TexCoord.y) * 2.0 - 1.0, depth, 1.0);
    const vec4 viewPosW = CB.invProjMat * clipPos;

    return vec3(CB.invViewMat * vec4(viewPosW.xyz / viewPosW.w, 1.0));
}

//============================================================================//

float sample_nearest_depth_float(sampler2D gbSampler, sampler2D smallDepthSampler, float threshold)
{
    const float depth = texture(tx_Depth, io_TexCoord).r;
    const float linearDepth = 1.0 / (depth * CB.invProjMat[2][3] + CB.invProjMat[3][3]);

    const vec4 depthGather = textureGather(smallDepthSampler, io_TexCoord);
    const vec4 linearDepthGather = 1.0 / (depthGather * CB.invProjMat[2][3] + CB.invProjMat[3][3]);

    const vec4 dists = abs(linearDepthGather - linearDepth);

    if (max(max(max(dists.x, dists.y), dists.z), dists.w) > threshold)
    {
        const vec4 gather = textureGather(gbSampler, io_TexCoord);
        float minDist = dists.x, result = gather.x;
        if (dists.y < minDist) { minDist = dists.y; result = gather.y; }
        if (dists.z < minDist) { minDist = dists.z; result = gather.z; }
        if (dists.w < minDist) { minDist = dists.w; result = gather.w; }
        return result;
    }
    return texture(gbSampler, io_TexCoord).r;
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

    const vec3 N = normalize(normal);
    const vec3 V = normalize(CB.invViewMat[3].xyz - get_world_position());

    const vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // non-zero to prevent black spots on normals pointing away from camera
    const float NdotV = max(dot(N, V), 0.0001);

    // indirect lighting
    {
        const vec3 F = fresnel_schlick_roughness(NdotV, F0, roughness);
        const vec2 lookup = texture(tx_BrdfLut, vec2(NdotV, 1.0 - roughness)).rg;
        const float ssao = 1.0;

        // todo: flip cube maps during load
        const vec3 irradiance = texture(tx_Irradiance, N * vec3(1,-1,1)).rgb;
        const vec3 radiance = textureLod(tx_Radiance, reflect(-V, N) * vec3(1,-1,1), roughness * 7.0).rgb;

        const vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo * irradiance;
        const vec3 specular = (F * lookup.x + lookup.y) * radiance;
        const vec3 fake = LB.ambiColour * albedo;

        frag_Colour = (diffuse + specular + fake) * ssao;
    }

    // direct lighting
    {
        const vec3 L = -LB.skyDirection;
        const vec3 H = normalize(V + L);

        const float NdotL = max(dot(N, L), 0.0);

        // cook torrance brdf
        const vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
        const float G = geometry_smith_schlick_ggx(NdotV, NdotL, roughness);
        const float D = distribution_ggx(N, H, roughness);

        const vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo / 3.14159265359;
        const vec3 specular = F * G * D / max(4.0 * NdotV * NdotL, 0.0001);

        frag_Colour += (diffuse + specular) * LB.skyColour * NdotL;
    }
}
