#version 450

#include "blocks/Camera.glsl"

layout(location=0) out vec3 io_CubeNorm;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    const vec2 ndcPos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0 - 1.0;
    const vec3 viewPos = vec3(CAMERA.invProjMat * vec4(ndcPos.x, -ndcPos.y, 0.0, 1.0));

    io_CubeNorm = mat3(CAMERA.invViewMat) * viewPos;
    io_CubeNorm.y = -io_CubeNorm.y;

    gl_Position = vec4(ndcPos.x, ndcPos.y, 0.0, 1.0);
}
