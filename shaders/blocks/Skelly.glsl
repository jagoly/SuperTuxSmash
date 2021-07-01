layout(std140, set=3, binding=0)
uniform ModelSkellyBlock
{
    mat4 matrix;
    mat3 normMat;
    mat3x4 bones[80];
}
MODEL;
