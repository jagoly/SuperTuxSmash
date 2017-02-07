// GLSL Fragment Super Shader

// #define OPT_TEX_DIFFUSE
// #define OPT_TEX_NORMAL
// #define OPT_TEX_SPECULAR
// #define OPT_COLOUR

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
layout(binding=0) uniform sampler2D tex_diffuse;
#else
uniform vec3 u_diffuse;
#endif

#ifdef OPT_TEX_NORMAL
layout(binding=1) uniform sampler2D tex_normal;
#endif

#ifdef OPT_TEX_SPECULAR
layout(binding=2) uniform sampler2D tex_specular;
#else
uniform vec3 u_specular;
#endif

#ifdef OPT_COLOUR
uniform vec3 u_colour;
#endif

//============================================================================//

layout(location=0) out vec3 frag_colour;

//============================================================================//

vec3 get_diffuse_value(vec3 diffuse, vec3 lightDir, vec3 normal)
{
    float factor = max(dot(-lightDir, normal), 0.f);
    return diffuse * factor;
}

vec3 get_specular_value(vec3 specular, float gloss, vec3 lightDir, vec3 normal)
{
    vec3 fromCam = normalize(-viewpos);
    vec3 reflection = reflect(lightDir, normal);
    float base = max(dot(fromCam, reflection), 0.f);
    float factor = pow(base, gloss * 100.f);
    return specular * factor;
}

//============================================================================//

void main() 
{
    #ifdef OPT_TEX_DIFFUSE
    const vec3 diffuse = texture(tex_diffuse, texcrd).rgb;
    #else
    const vec3 diffuse = u_diffuse;
    #endif

    #ifdef OPT_TEX_SPECULAR
    const vec3 specular = texture(tex_specular, texcrd).rgb;
    #else
    const vec3 specular = u_specular;
    #endif

    //========================================================//

    #ifdef OPT_TEX_NORMAL
    vec3 normal = normalize(texture(tex_normal, texcrd).rgb);
    normal = normalize(T * normal.x + B * normal.y + N * normal.z);
    #else
    const vec3 normal = normalize(N);
    #endif

    //========================================================//

    // calculate view space light direction
    vec3 lightDir = mat3(CB.view_mat) * normalize(LB.sky_direction);

    frag_colour = diffuse * LB.ambi_colour;
    frag_colour += get_diffuse_value(diffuse, lightDir, normal);
    frag_colour += get_specular_value(specular, 0.5f, lightDir, normal);

    //========================================================//

    #ifdef OPT_COLOUR
    frag_colour = mix(frag_colour, u_colour, 0.5f);
    #endif
}
