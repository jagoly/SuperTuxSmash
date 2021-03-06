// GLSL Fragment Shader

//============================================================================//

in vec2 texcrd;

layout(binding=0) uniform sampler2D tex_Main;

out vec4 frag_Colour;

//============================================================================//

vec3 tone_map(vec3 vec)
{
    const float A = 0.15f, B = 0.50f, C = 0.10f, D = 0.20f, E = 0.02f, F = 0.30f;
    return ((vec * (A * vec + C*B) + D*E) / (vec * (A * vec + B) + D*F)) - E/F;
}

//============================================================================//

void main()
{
    vec3 value = texture(tex_Main, texcrd).rgb;

    float sqrtLuma = sqrt(dot(vec3(0.22f, 0.69f, 0.09f), value));
    value = tone_map(value) / tone_map(vec3(1.f / sqrtLuma));

    //frag_Colour = vec4(value, dot(vec3(0.22f, 0.69f, 0.09f), value));
    frag_Colour = vec4(value, 1.f);
}
