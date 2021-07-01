layout(std140, set=0, binding=0)
uniform CameraBlock
{
    mat4 viewMat;
    mat4 projMat;
    mat4 invViewMat;
    mat4 invProjMat;
    mat4 projViewMat;
    vec3 position;
    vec3 direction;
}
CAMERA;
