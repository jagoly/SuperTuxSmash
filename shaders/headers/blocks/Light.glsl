// GLSL Uniform Block

struct LightBlock
{
    vec3 ambi_colour;   // 4
    vec3 sky_colour;    // 4
    vec3 sky_direction; // 4
    mat4 sky_matrix;    // 16

    // Size: 28
};
