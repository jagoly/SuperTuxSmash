#version 450

#include "headers/blocks/Camera.glsl"

layout(location=0) out vec3 io_CubeNorm;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    const vec2 ndcPos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0 - 1.0;
    const vec3 viewPos = vec3(CB.invProjMat * vec4(ndcPos, 0.0, 1.0));

    io_CubeNorm = normalize(mat3(CB.invViewMat) * viewPos);
    gl_Position = vec4(ndcPos, 0.0, 1.0);
}
