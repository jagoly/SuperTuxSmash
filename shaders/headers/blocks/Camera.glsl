// GLSL Uniform Block

struct CameraBlock
{
    mat4 view_mat; // 64
    mat4 proj_mat; // 64
    mat4 view_inv; // 64
    mat4 proj_inv; // 64
    vec3 pos;      // 16
    vec3 dir;      // 12

    // TOTAL: 284
};
