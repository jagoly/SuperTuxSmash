// GLSL Uniform Block

struct SkellyBlock
{
    mat4 matrix;      // 64
    mat3 normMat;     // 48
    mat3x4 bones[80]; // 3840

    // TOTAL: 3952
};

layout(std140, binding=2) uniform SKELLY { SkellyBlock SB; };
