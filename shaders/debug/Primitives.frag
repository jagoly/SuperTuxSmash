#version 450

layout(location=0) in vec4 io_Colour;

layout(location=0) out vec4 frag_Colour;

void main() 
{
    frag_Colour = io_Colour;
}
