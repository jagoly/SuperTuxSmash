// GLSL Uniform Block

struct EffectBlock
{
    mat4 matrix;     // 64
    mat3 normMat;    // 48
    vec4 params[8];  // 128
    mat3x4 bones[8]; // 384

    // TOTAL: 624
};

layout(std140, binding=2) uniform EFFECT { EffectBlock EB; };
