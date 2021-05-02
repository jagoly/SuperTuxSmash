#version 450

layout(push_constant) uniform PushConstants
{
    layout(offset=64) vec4 colour;
}
PC;

layout(location=0) out vec4 frag_Colour;

void main() 
{
    frag_Colour = PC.colour;
}
