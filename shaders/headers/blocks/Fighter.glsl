// GLSL Uniform Block

struct FighterBlock
{
    mat4 matrix;      // 64
    mat3 normMat;     // 48
    mat3x4 bones[80]; // 3840

    // TOTAL: 3952
};
