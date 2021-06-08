#version 450

layout(constant_id=0) const int DEBUG_MODE = 0;

layout(set=0, binding=0) uniform sampler2D tx_Colour;

layout(location=0) in vec2 io_TexCoord;

layout(location=0) out vec4 frag_Colour;

vec3 tone_map(vec3 vec)
{
    // todo: configurable tone mapping
    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
    return ((vec * (A * vec + C*B) + D*E) / (vec * (A * vec + B) + D*F)) - E/F;
}

void main()
{
    if (DEBUG_MODE == 0)
    {
        const vec3 texel = texture(tx_Colour, io_TexCoord).rgb;

        const float sqrtLuma = sqrt(dot(vec3(0.22, 0.69, 0.09), texel));
        const vec3 value = tone_map(texel) / tone_map(vec3(1.0 / sqrtLuma));

        //frag_Colour = vec4(value, dot(vec3(0.22, 0.69, 0.09), value));
        frag_Colour = vec4(value, 1.0);
    }

    else if (DEBUG_MODE == 1) // rgb
        frag_Colour = vec4(texture(tx_Colour, io_TexCoord).rgb, 1.0);

    else if (DEBUG_MODE == 2) // alpha
        frag_Colour = vec4(texture(tx_Colour, io_TexCoord).aaa, 1.0);

    else if (DEBUG_MODE == 3) // red
        frag_Colour = vec4(texture(tx_Colour, io_TexCoord).rrr, 1.0);

    else if (DEBUG_MODE == 4) // normals
        frag_Colour = vec4(texture(tx_Colour, io_TexCoord).rgb * 0.5 + 0.5, 1.0);
}
