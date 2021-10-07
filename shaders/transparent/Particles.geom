#version 450

//============================================================================//

layout(set=0, binding=0, std140)
#include "../blocks/Camera.glsl"

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

layout(location=0) in VertexBlock
{
    vec3 viewPos;
    float radius;
    vec3 colour;
    float opacity;
    float layer;
}
IN[];

layout(location=0) out GeometryBlock
{
    vec3 texCoord;
    float nearDepth;
    vec3 colour;
    float opacity;
}
OUT;

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void emit_vertex(float u, float v)
{
    const vec2 xy = vec2(u, v) * 2.0 - 1.0;
    const vec2 viewPos = IN[0].viewPos.xy + xy * vec2(IN[0].radius);

    gl_Position = CAMERA.projMat * vec4(viewPos, IN[0].viewPos.z, 1.0);
    OUT.texCoord = vec3(u, v, IN[0].layer);
    OUT.nearDepth = IN[0].viewPos.z - IN[0].radius;
    OUT.colour = IN[0].colour;
    OUT.opacity = IN[0].opacity;

    EmitVertex();
}

//============================================================================//

void main()
{
    emit_vertex(0.0, 0.0);
    emit_vertex(0.0, 1.0);
    emit_vertex(1.0, 0.0);
    emit_vertex(1.0, 1.0);

    EndPrimitive();
}   
