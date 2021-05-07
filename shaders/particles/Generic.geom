#version 450

#include "../headers/blocks/Camera.glsl"

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

layout(location=0) in VertexBlock
{
    vec3 viewPos;
    float radius;
    vec3 colour;
    float opacity;
    float index;
}
IN[];

layout(location=0) out GeometryBlock
{
    vec3 texCoord;
    float nearDepth;
    vec3 colour;
    float opacity;
    vec2 fragCoord;
}
OUT;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    const vec3 offsetX = vec3(IN[0].radius, 0.0, 0.0);
    const vec3 offsetY = vec3(0.0, IN[0].radius, 0.0);

    const vec3 viewPosA = IN[0].viewPos + (-offsetX -offsetY) * IN[0].radius;
    const vec3 viewPosB = IN[0].viewPos + (-offsetX +offsetY) * IN[0].radius;
    const vec3 viewPosC = IN[0].viewPos + (+offsetX -offsetY) * IN[0].radius;
    const vec3 viewPosD = IN[0].viewPos + (+offsetX +offsetY) * IN[0].radius;
  
    gl_Position = CB.projMat * vec4(viewPosA, 1.0);
    OUT.texCoord = vec3(0.0, 0.0, IN[0].index);
    OUT.nearDepth = viewPosA.z - IN[0].radius;
    OUT.colour = IN[0].colour;
    OUT.opacity = IN[0].opacity;
    OUT.fragCoord = gl_Position.xy * vec2(0.5, -0.5) + 1.0;
    EmitVertex();

    gl_Position = CB.projMat * vec4(viewPosB, 1.0);
    OUT.texCoord = vec3(0.0, 1.0, IN[0].index);
    OUT.nearDepth = viewPosB.z - IN[0].radius;
    OUT.colour = IN[0].colour;
    OUT.opacity = IN[0].opacity;
    OUT.fragCoord = gl_Position.xy * vec2(0.5, -0.5) + 1.0;
    EmitVertex();
  
    gl_Position = CB.projMat * vec4(viewPosC, 1.0);
    OUT.texCoord = vec3(1.0, 0.0, IN[0].index);
    OUT.nearDepth = viewPosC.z - IN[0].radius;
    OUT.colour = IN[0].colour;
    OUT.opacity = IN[0].opacity;
    OUT.fragCoord = gl_Position.xy * vec2(0.5, -0.5) + 1.0;
    EmitVertex();
  
    gl_Position = CB.projMat * vec4(viewPosD, 1.0);
    OUT.texCoord = vec3(1.0, 1.0, IN[0].index);
    OUT.nearDepth = viewPosD.z - IN[0].radius;
    OUT.colour = IN[0].colour;
    OUT.opacity = IN[0].opacity;
    OUT.fragCoord = gl_Position.xy * vec2(0.5, -0.5) + 1.0;
    EmitVertex();

    EndPrimitive();  
}   
