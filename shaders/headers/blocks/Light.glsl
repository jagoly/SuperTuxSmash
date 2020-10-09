// GLSL Uniform Block

struct LightBlock
{
    vec3 ambiColour;   // 16
    vec3 skyColour;    // 16
    vec3 skyDirection; // 16
    mat4 skyMatrix;    // 64

    // TOTAL: 112
};

layout(std140, binding=1) uniform LIGHT { LightBlock LB; };
