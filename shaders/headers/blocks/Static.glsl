// GLSL Uniform Block

layout(std140, set=3, binding=0) uniform StaticBlock
{
    mat4 matrix;
    mat3 normMat;
}
SB;
