// GLSL Vertex Super Shader

//============================================================================//

#include headers/blocks/Camera

layout(std140, binding=0) uniform CAMERA { CameraBlock CB; };

//============================================================================//

layout(location=0) in vec3 v_Position;
layout(location=1) in vec2 v_TexCoord;
layout(location=2) in vec3 v_Normal;
layout(location=3) in vec4 v_Tangent;

layout(location=0) uniform mat4 u_Matrix;
layout(location=1) uniform mat3 u_NormMat;

out vec2 texcrd;
out vec3 viewpos;
out vec3 N, T, B;

//============================================================================//

void main() 
{
    texcrd = v_TexCoord;

    N = normalize(u_NormMat * v_Normal);
    T = normalize(u_NormMat * v_Tangent.xyz);
    B = normalize(u_NormMat * cross(v_Normal, v_Tangent.xyz) * v_Tangent.w);
    
    viewpos = vec3(CB.invProjMat * u_Matrix * vec4(v_Position, 1.f));

    gl_Position = u_Matrix * vec4(v_Position, 1.f);
}
