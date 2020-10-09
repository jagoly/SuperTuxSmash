// GLSL Uniform Block

struct StaticBlock
{
    mat4 matrix;      // 64
    mat3 normMat;     // 48

    // TOTAL: 112
};

layout(std140, binding=2) uniform STATIC { StaticBlock SB; };
