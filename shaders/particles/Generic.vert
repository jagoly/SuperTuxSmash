#version 450

#include "../headers/blocks/Camera.glsl"

layout(location=0) in vec3 v_Position;
layout(location=1) in float v_Radius;
layout(location=2) in vec3 v_Colour;
layout(location=3) in float v_Opacity;
layout(location=4) in float v_Index;
layout(location=5) in float v_Padding;

layout(location=0) out VertexBlock
{
    vec3 viewPos;
    float radius;
    vec3 colour;
    float opacity;
    float index;
}
OUT;

void main() 
{
    OUT.viewPos = vec3(CB.viewMat * vec4(v_Position, 1.0));
    OUT.radius = v_Radius;
    OUT.colour = v_Colour;
    OUT.opacity = v_Opacity;
    OUT.index = v_Index;
}
