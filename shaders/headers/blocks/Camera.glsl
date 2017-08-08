// GLSL Uniform Block

struct CameraBlock
{
    mat4 viewMat;    // 64
    mat4 projMat;    // 64
    mat4 invViewMat; // 64
    mat4 invProjMat; // 64
    vec3 position;   // 16
    vec3 direction;  // 16

    // TOTAL: 288
};
