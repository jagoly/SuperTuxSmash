// GLSL Fragment Super Shader

// #define OPT_TEX_DIFFUSE
// #define OPT_TEX_NORMAL
// #define OPT_TEX_SPECULAR
// #define OPT_SUB_SCATTER

//============================================================================//

#include headers/blocks/Camera
#include headers/blocks/Light

layout(std140, binding=0) uniform CAMERABLOCK { CameraBlock CB; };
layout(std140, binding=1) uniform LIGHTBLOCK { LightBlock LB; };

//============================================================================//

in vec2 texcrd;
in vec3 viewpos;
in vec3 N, T, B;

//============================================================================//

#ifdef OPT_TEX_DIFFUSE
layout(binding=0) uniform sampler2D tex_Diffuse;
#else
layout(location=2) uniform vec3 u_Diffuse;
#endif

#ifdef OPT_TEX_NORMAL
layout(binding=1) uniform sampler2D tex_Normal;
#endif

#ifdef OPT_TEX_SPECULAR
layout(binding=2) uniform sampler2D tex_Specular;
#else
layout(location=3) uniform vec3 u_Specular;
#endif

//============================================================================//

layout(location=0) out vec3 frag_Colour;

//============================================================================//

vec3 get_diffuse_value(vec3 colour, vec3 lightDir, vec3 normal)
{
    #ifdef OPT_SUB_SCATTER
    const float wrap = 0.5f;
    float factor = (dot(-lightDir, normal) + wrap) / (1.f + wrap);
    #else
    float factor = dot(-lightDir, normal);
    #endif

    return colour * max(factor, 0.f);
}

vec3 get_specular_value(vec3 colour, float gloss, vec3 lightDir, vec3 normal)
{
    vec3 reflection = reflect(lightDir, normal);
    vec3 dirFromCam = normalize(-viewpos);

    float factor = max(dot(dirFromCam, reflection), 0.f);
    factor = pow(factor, gloss * 100.f);

    return colour * factor;
}

//============================================================================//

void main() 
{
    #ifdef OPT_TEX_DIFFUSE
    const vec3 diffuse = texture(tex_Diffuse, texcrd).rgb;
    #else
    const vec3 diffuse = u_Diffuse;
    #endif

    //--------------------------------------------------------//

    #ifdef OPT_TEX_SPECULAR
    const vec3 specular = texture(tex_Specular, texcrd).rgb;
    #else
    const vec3 specular = u_Specular;
    #endif

    //--------------------------------------------------------//

    #ifdef OPT_TEX_NORMAL
    const vec3 nm = normalize(texture(tex_Normal, texcrd).rgb);
    const vec3 normal = normalize(T * nm.x + B * nm.y + N * nm.z);
    #else
    const vec3 normal = normalize(N);
    #endif

    //--------------------------------------------------------//

    const vec3 lightDir = mat3(CB.viewMat) * normalize(LB.skyDirection);

    frag_Colour = diffuse * LB.ambiColour;
    frag_Colour += get_diffuse_value(diffuse, lightDir, normal) * LB.skyColour;
    frag_Colour += get_specular_value(specular, 0.5f, lightDir, normal) * LB.skyColour;
}
