#version 450

layout(set=0, binding=0) uniform sampler2D tx_Colour;

layout(location=0) in vec2 io_TexCoord;

layout(location=0) out vec4 frag_Colour;

vec3 tone_map(vec3 vec)
{
    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
    return ((vec * (A * vec + C*B) + D*E) / (vec * (A * vec + B) + D*F)) - E/F;
}

void main()
{
    vec3 value = texture(tx_Colour, io_TexCoord).rgb;

    float sqrtLuma = sqrt(dot(vec3(0.22, 0.69, 0.09), value));
    value = tone_map(value) / tone_map(vec3(1.0 / sqrtLuma));

    //frag_Colour = vec4(value, dot(vec3(0.22, 0.69, 0.09), value));
    frag_Colour = vec4(value, 1.0);
}
