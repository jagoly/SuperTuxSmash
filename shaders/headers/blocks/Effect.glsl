// GLSL Uniform Block

layout(std140, set=3, binding=0) uniform EffectBlock
{
    mat4 matrix;
    mat3 normMat;
    vec4 params[8];
    mat3x4 bones[8];
}
EB;
