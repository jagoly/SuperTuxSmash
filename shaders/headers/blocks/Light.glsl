// GLSL Uniform Block

struct LightBlock
{
    vec3 ambi_colour;   // 16
    vec3 sky_colour;    // 16
    vec3 sky_direction; // 16
    mat4 sky_matrix;    // 64

    // TOTAL: 112
};
