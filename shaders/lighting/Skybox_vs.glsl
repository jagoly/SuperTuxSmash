// GLSL Vertex Shader

//============================================================================//

#include builtin/misc/screen

#include headers/blocks/Camera

//============================================================================//

layout(std140, binding=0) uniform CAMERA { CameraBlock CB; };

out vec3 cubeNorm;

//============================================================================//

void main()
{
    vec3 viewPos = vec3(CB.invProjMat * vec4(v_Position, 0.f, 1.f));
    cubeNorm = normalize(mat3(CB.invViewMat) * viewPos);

    gl_Position = vec4(v_Position, 0.f, 1.f);
}
