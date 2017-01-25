// GLSL Uniform Block

struct CameraBlock
{
    mat4 view_mat; // 16
    mat4 proj_mat; // 16
    mat4 view_inv; // 16
    mat4 proj_inv; // 16
    vec3 pos;      // 4
    vec3 dir;      // 4

    // Size: 72
};
