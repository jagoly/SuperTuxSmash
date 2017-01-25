// GLSL Vertex Super Shader

// #define OPT_SKELETON

//============================================================================//

#include headers/blocks/Camera

layout(std140, binding=0) uniform CAMERABLOCK { CameraBlock CB; };

//============================================================================//

layout(location=0) in vec3 V_pos;
layout(location=1) in vec2 V_tcrd;
layout(location=2) in vec3 V_norm;
layout(location=3) in vec4 V_tan;

uniform mat4 u_model_mat;
uniform mat3 u_normal_mat;

out vec2 texcrd;
out vec3 viewpos;
out vec3 N, T, B;

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main() 
{
    texcrd = V_tcrd;

    N = normalize(u_normal_mat * V_norm);
    T = normalize(u_normal_mat * V_tan.xyz);
    B = normalize(u_normal_mat * cross(V_norm, V_tan.xyz) * V_tan.w);

    viewpos = vec3(CB.view_mat * u_model_mat * vec4(V_pos, 1.f));
    gl_Position = CB.proj_mat * CB.view_mat * u_model_mat * vec4(V_pos, 1.f);
}
