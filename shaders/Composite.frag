#version 450

layout(constant_id=0) const int DEBUG_MODE = 0;

layout(push_constant) uniform PushConstants
{
    float exposure;
    float contrast;
    float black;
}
PC;

layout(set=0, binding=0) uniform sampler2D tx_Colour;

layout(location=0) in vec2 io_TexCoord;

layout(location=0) out vec4 frag_Colour;

//============================================================================//

// TODO: Because offsreen target sizes are rounded down to even numbers, nearest-neighbour filtering
//       causes double pixel lines to appear in the centre of the image. This should be fixed so that
//       the lines appear at the top and right instead. Low priority since its barely noticable and
//       only ever happens if the user manually resizes the window.

//============================================================================//

vec3 convert_rgb_to_xyY(vec3 rgb)
{
    const float x = dot(vec3(0.4124564, 0.3575761, 0.1804375), rgb);
    const float y = dot(vec3(0.2126729, 0.7151522, 0.0721750), rgb);
    const float z = dot(vec3(0.0193339, 0.1191920, 0.9503041), rgb);

    return vec3(vec2(x, y) / (x + y + z), y);
}

vec3 convert_xyY_to_rgb(vec3 xyY)
{
    const vec3 xyz = vec3(vec2(xyY.x, 1.0 - xyY.x - xyY.y) * (xyY.z / xyY.y), xyY.z).xzy;

    const float r = dot(vec3(+3.2404542, -1.5371385, -0.4985314), xyz);
    const float g = dot(vec3(-0.9692660, +1.8760108, +0.0415560), xyz);
    const float b = dot(vec3(+0.0556434, -0.2040259, +1.0572252), xyz);

    return vec3(r, g, b);
}

//============================================================================//

//float tone_map_reinhard(float Y)
//{
//    const float whitePoint = 3.0;
//    return (Y * (1.0 + Y / (whitePoint * whitePoint))) / (1.0 + Y);
//}

//float tone_map_aces_Y(float Y)
//{
//    const float A = 2.51, B = 0.03, C = 2.43, D = 0.59, E = 0.14;
//    return (Y * (A * Y + B)) / (Y * (C * Y + D) + E);
//}

//vec3 tone_map_aces_rgb(vec3 rgb)
//{
//    const float A = 2.51, B = 0.03, C = 2.43, D = 0.59, E = 0.14;
//    return (rgb * (A * rgb + B)) / (rgb * (C * rgb + D) + E);
//}

//============================================================================//

float tone_map_uchimura(float x, float P, float a, float m, float l, float c, float b)
{
    // Uchimura 2017, "HDR theory and practice"
    // Math: https://www.desmos.com/calculator/gslcdxvipg
    // Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp

    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float w0 = 1.0 - smoothstep(0.0, m, x);
    float w2 = step(m + l0, x);
    float w1 = 1.0 - w0 - w2;

    float T = m * pow(x / m, c) + b;
    float S = P - (P - S1) * exp(CP * (x - S0));
    float L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

float tone_map_uchimura_Y(float Y)
{
    const float maxBrightness = 1.0;
    const float linearStart = 0.22;
    const float linearLength = 0.4;
    const float pedestal = 0.0;

    return tone_map_uchimura(Y, maxBrightness, PC.contrast, linearStart, linearLength, PC.black, pedestal);
}

vec3 tone_map_uchimura_rgb(vec3 rgb)
{
    const float r = tone_map_uchimura_Y(rgb.r);
    const float g = tone_map_uchimura_Y(rgb.g);
    const float b = tone_map_uchimura_Y(rgb.b);

    return vec3(r, g, b);
}

//============================================================================//

void main()
{
    if (DEBUG_MODE == 0)
    {
        const vec3 texel = texture(tx_Colour, io_TexCoord).rgb;

        vec3 xyY = convert_rgb_to_xyY(texel);
        xyY.z = xyY.z * PC.exposure;

        //xyY.z = tone_map_reinhard(xyY.z);
        //xyY.z = tone_map_aces_Y(xyY.z);

        vec3 rgb = convert_xyY_to_rgb(xyY);

        //rgb = tone_map_aces_rgb(rgb);
        rgb = tone_map_uchimura_rgb(rgb);

        frag_Colour = vec4(rgb, 1.0);
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
