// GLSL Fragment Shader

//============================================================================//

in vec2 texcrd;

layout(binding=0) uniform sampler2D texMain;

out vec4 fragColour;

//============================================================================//

vec3 tone_map(vec3 texel)
{
    float A = 0.15f; float B = 0.50f; float C = 0.10f;
    float D = 0.20f; float E = 0.02f; float F = 0.30f;
    vec3 num = texel * (A * texel + C * B) + D * E;
    vec3 den = texel * (A * texel + B) + D * F;
    return (num / den) - (E / F);
}

//============================================================================//

void main()
{
    vec3 value = texture(texMain, texcrd).rgb;

    float sqrtLuma = sqrt(dot(vec3(0.22f, 0.69f, 0.09f), value));
    value = tone_map(value) / tone_map(vec3(1.f / sqrtLuma));

//    fragColour = vec4(value, dot(vec3(0.22f, 0.69f, 0.09f), value));
    fragColour = vec4(value, 1.f);
}
